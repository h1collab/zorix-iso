#!/usr/bin/python3
# SPDX-License-Identifier: MIT
"""Local Zorix desktop broker. Loopback only; no remote shell API.

The browser is a user-space desktop surface. All writes require a per-session
secret, checked Host and Origin. Files are confined to the user's home, notes
have one fixed path, and executable launching is a strict allowlist.
"""
from __future__ import annotations
import argparse, base64, hashlib, hmac, json, mimetypes, os, pathlib, secrets, shutil
import signal, socket, subprocess, sys, threading, time, urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

APPS = {
 'files': ['/usr/bin/zorix-files'],
 'terminal': ['/usr/bin/xterm','-fa','Zorix Sans','-fs','12','-bg','#0c1321','-fg','#e9f4ff','-T','Zorix Terminal'],
 'windows': ['/usr/bin/zorix-windows'],
 'gpu': ['/usr/bin/zorix-gpu-center'],
 'browser': ['/usr/bin/chromium','--user-data-dir={home}/.config/zorix/browser','--no-first-run','--disable-sync','--disable-background-networking','https://www.debian.org/'],
 'ai': ['/usr/bin/zorix-ai'],
 'text': ['/usr/bin/zorix-text'],
 'image': ['/usr/bin/zorix-image'],
 'archive': ['/usr/bin/zorix-archive'],
 'monitor': ['/usr/bin/zorix-monitor'],
 'software': ['/usr/bin/zorix-software'],
 'media': ['/usr/bin/zorix-media'],
 'network': ['/usr/bin/nm-connection-editor'],
 'bluetooth': ['/usr/bin/blueman-manager'],
 'audio': ['/usr/bin/pavucontrol'],
}
def serial(message):
 try:
  with open('/dev/ttyS0','w',encoding='utf-8',buffering=1) as f:
   f.write(str(message)+'\n')
 except OSError:
  pass

DEFAULT = {'theme':'aurora', 'reducedMotion':False, 'reducedTransparency':False, 'contrast':False, 'language':'en', 'welcomeDone':False, 'glassIntensity':64, 'wallpaperMotion':False, 'largeText':False}

class State:
 def __init__(self, home:pathlib.Path, ui:pathlib.Path, core:pathlib.Path, testing=False):
  self.home=home.resolve(); self.ui=ui.resolve(); self.core=core; self.testing=testing
  self.token=secrets.token_urlsafe(32); self.lock=threading.Lock()
  self.config=self.home/'.config/zorix'; self.config.mkdir(parents=True,exist_ok=True,mode=0o700)
  self.notes=self.home/'Documents/Zorix Notes.txt'; self.notes.parent.mkdir(parents=True,exist_ok=True)
  self.tasks=self.config/'tasks.json'
  self.audit=self.config/'actions.jsonl'; self.events=[]; self.desktop_map={}
 def settings(self):
  result=DEFAULT.copy()
  try:
   data=json.loads((self.config/'glass.json').read_text())
   result.update({k:v for k,v in data.items() if k in DEFAULT})
  except (OSError,ValueError):pass
  return result
 def save(self,path,data):
  if path.is_symlink():raise ValueError('Symbolic-link destinations are not allowed')
  tmp=path.parent/('.zorix-'+secrets.token_hex(8))
  fd=os.open(tmp,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
  try:
   with os.fdopen(fd,'w',encoding='utf-8') as f:f.write(data);f.flush();os.fsync(f.fileno())
   os.replace(tmp,path)
  finally:
   try:tmp.unlink()
   except FileNotFoundError:pass
 def record(self,action):
  entry={'time':time.time(),'action':action}
  self.events.append(entry);self.events=self.events[-30:]
  # Do not log tokens, note contents, or private filenames.
 def safe_path(self,name):
  if not isinstance(name,str) or len(name)>2048:raise ValueError('Invalid path')
  p=(self.home/name).resolve()
  if not p.is_relative_to(self.home):raise ValueError('Path must remain inside your home folder')
  return p
 def system(self):
  try:
   p=subprocess.run([str(self.core)],capture_output=True,text=True,timeout=4,check=True)
   result=json.loads(p.stdout)
  except (OSError,ValueError,subprocess.SubprocessError) as e:
   result={'system':'Zorix OS','version':'1.3.0','runtime':'Swift component unavailable','error':str(e)[:160]}
  result['capabilities']={k:os.access(v[0],os.X_OK) for k,v in APPS.items()}
  result['networkInterfaces']=[p.name for p in pathlib.Path('/sys/class/net').glob('*') if p.name!='lo']
  result['storagePersistence']='volatile' if pathlib.Path('/etc/zorix-live').exists() else 'host-user-folder'
  rust=pathlib.Path('/usr/bin/zorix-rust-core')
  result['rust']={'sourceIntegrated':True,'binaryAvailable':os.access(rust,os.X_OK),'runtime':'compiled' if os.access(rust,os.X_OK) else 'source-only in this build'}
  result['wineInstalled']=shutil.which('wine') is not None
  result['nvidiaDriverPresent']=pathlib.Path('/proc/driver/nvidia/version').exists()
  try:
   product=pathlib.Path('/sys/class/dmi/id/product_name').read_text().strip()
   vendor=pathlib.Path('/sys/class/dmi/id/sys_vendor').read_text().strip()
  except OSError: product=vendor=''
  ident=(vendor+' '+product).lower()
  if 'virtualbox' in ident or 'innotek' in ident: platform='VirtualBox'
  elif 'vmware' in ident: platform='VMware'
  elif 'qemu' in ident or 'kvm' in ident: platform='QEMU/KVM'
  elif 'microsoft' in ident and 'virtual' in ident: platform='Hyper-V'
  else: platform=(vendor+' '+product).strip() or 'unknown'
  result['platform']=platform
  result['displayStack']={name: shutil.which(name) is not None for name in ('Xvfb','Xorg','openbox','chromium','xdpyinfo')}
  try:
   st=os.statvfs(self.home); result['homeStorage']={'totalBytes':st.f_blocks*st.f_frsize,'freeBytes':st.f_bavail*st.f_frsize}
  except OSError: result['homeStorage']={}
  return result
 def desktop_apps(self):
  result=[]; mapping={}
  for base in (pathlib.Path('/usr/share/applications'),self.home/'.local/share/applications'):
   if not base.is_dir(): continue
   for p in sorted(base.glob('*.desktop')):
    try:
     fields={}; active=False
     for raw in p.read_text(errors='replace').splitlines():
      line=raw.strip()
      if line.startswith('[') and line.endswith(']'):
       active=line=='[Desktop Entry]'; continue
      if not active or not line or line.startswith('#') or '=' not in line: continue
      k,v=line.split('=',1)
      if k in ('Name','Comment','Type','Hidden','NoDisplay'): fields.setdefault(k,v.strip())
     if fields.get('Type','Application')!='Application' or fields.get('Hidden','false').lower()=='true' or fields.get('NoDisplay','false').lower()=='true': continue
     name=fields.get('Name') or p.stem
     ident='desktop:'+hashlib.sha256(str(p).encode()).hexdigest()[:16]
     mapping[ident]=p
     result.append({'id':ident,'name':name[:80],'comment':fields.get('Comment','Installed application')[:120]})
    except OSError: continue
  self.desktop_map=mapping
  return sorted(result,key=lambda x:x['name'].casefold())[:160]
 def connectivity(self):
  route_iface=''
  try:
   for line in pathlib.Path('/proc/net/route').read_text(errors='replace').splitlines()[1:]:
    cols=line.split()
    if len(cols)>3 and cols[1]=='00000000' and (int(cols[3],16)&2):
     route_iface=cols[0]; break
  except (OSError,ValueError): pass
  devices=[]; wifi_present=False
  for p in sorted(pathlib.Path('/sys/class/net').glob('*')):
   if p.name=='lo': continue
   try: state=(p/'operstate').read_text().strip()
   except OSError: state='unknown'
   wireless=(p/'wireless').exists(); wifi_present=wifi_present or wireless
   devices.append({'name':p.name,'state':state,'wireless':wireless})
  network={'connected':bool(route_iface),'defaultRoute':bool(route_iface),'interface':route_iface,'connection':'','ssid':'','wifiPresent':wifi_present,'wifiEnabled':None,'devices':devices}
  if shutil.which('nmcli'):
   try:
    p=subprocess.run(['nmcli','-t','--escape','no','-f','DEVICE,TYPE,STATE,CONNECTION','device','status'],capture_output=True,text=True,timeout=2)
    for line in p.stdout.splitlines():
     parts=line.split(':',3)
     if len(parts)<4: continue
     dev,typ,state,conn=parts
     if state.startswith('connected'):
      network['connected']=True
      if not network['interface']: network['interface']=dev
      if not network['connection']: network['connection']=conn
      if typ=='wifi': network['ssid']=conn
    q=subprocess.run(['nmcli','-t','-f','WIFI','general'],capture_output=True,text=True,timeout=2)
    if q.returncode==0: network['wifiEnabled']=q.stdout.strip().lower()=='enabled'
   except (OSError,subprocess.SubprocessError): pass
  bt={'present':False,'powered':False,'adapter':''}
  adapters=sorted(pathlib.Path('/sys/class/bluetooth').glob('hci*'))
  if adapters: bt={'present':True,'powered':False,'adapter':adapters[0].name}
  if shutil.which('bluetoothctl'):
   try:
    p=subprocess.run(['bluetoothctl','show'],capture_output=True,text=True,timeout=2)
    for line in p.stdout.splitlines():
     q=line.strip()
     if q.startswith('Controller '):
      bt['present']=True; parts=q.split(); bt['adapter']=parts[1] if len(parts)>1 else bt['adapter']
     elif q.startswith('Powered:'): bt['powered']=q.split(':',1)[1].strip().lower()=='yes'
   except (OSError,subprocess.SubprocessError): pass
  audio={'available':False,'server':''}
  if shutil.which('pactl'):
   try:
    p=subprocess.run(['pactl','info'],capture_output=True,text=True,timeout=2)
    if p.returncode==0:
     audio['available']=True
     for line in p.stdout.splitlines():
      if line.startswith('Server Name:'): audio['server']=line.split(':',1)[1].strip()[:100]
   except (OSError,subprocess.SubprocessError): pass
  battery={'present':False,'name':'','capacity':0,'status':''}
  for bat in pathlib.Path('/sys/class/power_supply').glob('*'):
   try:
    if (bat/'type').read_text().strip()!='Battery': continue
    battery={'present':True,'name':bat.name,'capacity':int((bat/'capacity').read_text().strip()),'status':(bat/'status').read_text().strip()}; break
   except (OSError,ValueError): continue
  return {'network':network,'bluetooth':bt,'audio':audio,'battery':battery}
 def mounted_volumes(self):
  roots=[]
  for base in (pathlib.Path('/media')/self.home.name,pathlib.Path('/run/media')/self.home.name):
   if not base.is_dir():continue
   try:
    for p in sorted(base.iterdir(),key=lambda x:x.name.casefold()):
     try:
      if not p.is_dir() or p.is_symlink():continue
      ident='volume:'+hashlib.sha256(str(p.resolve()).encode()).hexdigest()[:16]
      roots.append({'id':ident,'name':p.name[:80],'path':str(p.resolve())})
     except OSError:continue
   except OSError:continue
  return roots[:32]
 def volume_path(self,ident):
  if not isinstance(ident,str) or not ident.startswith('volume:'):raise ValueError('Invalid volume id')
  for item in self.mounted_volumes():
   if item['id']==ident:
    p=pathlib.Path(item['path']).resolve()
    for base in (pathlib.Path('/media').resolve(),pathlib.Path('/run/media').resolve()):
     if p.is_relative_to(base):return p
  raise ValueError('Mounted volume not found')
 def task_list(self):
  try:
   data=json.loads(self.tasks.read_text())
   if isinstance(data,list):return [x for x in data if isinstance(x,dict)][:60]
  except (OSError,ValueError):pass
  return []
 def process_list(self):
  rows=[]
  for d in pathlib.Path('/proc').glob('[0-9]*'):
   try:
    status={}
    for line in (d/'status').read_text(errors='replace').splitlines():
     if ':' in line:
      k,v=line.split(':',1);status[k]=v.strip()
    rows.append({'pid':int(d.name),'name':status.get('Name','?')[:80],'state':status.get('State','?')[:32],'rssKiB':int(status.get('VmRSS','0 kB').split()[0]),'threads':int(status.get('Threads','0'))})
   except (OSError,ValueError,IndexError):continue
  return sorted(rows,key=lambda x:x['rssKiB'],reverse=True)[:32]
 def hardware(self):
  gpu=[]
  for card in sorted(pathlib.Path('/sys/class/drm').glob('card[0-9]')):
   dev=card/'device'
   try:
    driver=(dev/'driver').resolve().name if (dev/'driver').exists() else 'unknown'
    vendor=(dev/'vendor').read_text().strip() if (dev/'vendor').exists() else ''
    device=(dev/'device').read_text().strip() if (dev/'device').exists() else ''
    gpu.append({'card':card.name,'driver':driver,'vendor':vendor,'device':device})
   except OSError:continue
  battery=[]
  for bat in pathlib.Path('/sys/class/power_supply').glob('*'):
   try:
    typ=(bat/'type').read_text().strip()
    if typ=='Battery':battery.append({'name':bat.name,'capacity':int((bat/'capacity').read_text().strip()),'status':(bat/'status').read_text().strip()})
   except (OSError,ValueError):continue
  temps=[]
  for zone in list(pathlib.Path('/sys/class/thermal').glob('thermal_zone*'))[:12]:
   try:
    name=(zone/'type').read_text().strip(); raw=int((zone/'temp').read_text().strip()); temps.append({'name':name[:80],'celsius':round(raw/1000,1)})
   except (OSError,ValueError):continue
  return {'gpus':gpu,'batteries':battery,'temperatures':temps}

class Handler(BaseHTTPRequestHandler):
 server_version='ZorixGlass/1.3.0'
 def log_message(self,*args):pass
 @property
 def s(self):return self.server.state
 def send(self,code,data,kind='application/json'):
  if not isinstance(data,bytes):data=json.dumps(data,ensure_ascii=False).encode()
  self.send_response(code);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)))
  self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff')
  self.send_header('Referrer-Policy','no-referrer');self.send_header('X-Frame-Options','DENY')
  self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self'; connect-src 'self'; frame-ancestors 'none'; object-src 'none'; base-uri 'none'")
  try:
   self.end_headers();self.wfile.write(data)
  except (BrokenPipeError,ConnectionResetError):
   return
 def allowed_host(self):
  return self.headers.get('Host','')==f'127.0.0.1:{self.server.server_port}'
 def auth(self):
  if not self.allowed_host():return False
  origin=self.headers.get('Origin')
  if origin and origin!=f'http://127.0.0.1:{self.server.server_port}':return False
  try:return hmac.compare_digest(self.headers.get('X-Zorix-Token',''),self.s.token)
  except TypeError:return False
 def do_GET(self):
  if not self.allowed_host():return self.send(403,{'error':'Invalid host'})
  parsed=urllib.parse.urlsplit(self.path); path=parsed.path
  try:
   if path=='/zorix-ui.ttf':
    font=pathlib.Path(os.environ.get('ZORIX_TEST_FONT','/run/zorix-fonts/ZorixSans.ttf'))
    return self.send(200,font.read_bytes(),'font/ttf')
   if path.startswith('/api/'):
    if not self.auth():
     serial('ZORIX_AUTH_FAIL:GET:'+path+':token=' + ('yes' if bool(self.headers.get('X-Zorix-Token')) else 'no'))
     return self.send(403,{'error':'Session authorization required'})
    if path=='/api/system':return self.send(200,self.s.system())
    if path=='/api/connectivity':return self.send(200,self.s.connectivity())
    if path=='/api/apps':return self.send(200,{'apps':self.s.desktop_apps()})
    if path=='/api/volumes':return self.send(200,{'volumes':self.s.mounted_volumes()})
    if path=='/api/settings':return self.send(200,self.s.settings())
    if path=='/api/notes':
     self.s.safe_path(str(self.s.notes))
     if self.s.notes.is_symlink():raise ValueError('Note file is a symbolic link')
     return self.send(200,{'text':self.s.notes.read_text()[:200000] if self.s.notes.exists() else ''})
    if path=='/api/files':
     qs=urllib.parse.parse_qs(parsed.query); name=qs.get('path',[''])[0]; rootid=qs.get('root',['home'])[0]
     if rootid=='home':
      root=self.s.home
     else:
      root=self.s.volume_path(rootid)
     p=(root/name).resolve()
     if not p.is_relative_to(root):raise ValueError('Path must remain inside the selected root')
     if not p.is_dir():raise ValueError('Not a directory')
     result=[]
     for x in sorted(p.iterdir(),key=lambda p:(not p.is_dir(),p.name.casefold())):
      if x.name.startswith('.'):continue
      try:
       result.append({'name':x.name,'directory':x.is_dir(),'size':x.stat().st_size,'link':x.is_symlink()})
      except OSError:continue
      if len(result)==500:break
     return self.send(200,{'root':rootid,'path':str(p.relative_to(root)),'items':result})
    if path=='/api/events':return self.send(200,{'events':self.s.events})
    if path=='/api/tasks':return self.send(200,{'tasks':self.s.task_list()})
    if path=='/api/processes':return self.send(200,{'processes':self.s.process_list()})
    if path=='/api/hardware':return self.send(200,self.s.hardware())
    return self.send(404,{'error':'Unknown API'})
   path='/index.html' if path=='/' else path
   if path in ('/index.html','/style.css','/app.js'):
    print('ZORIX_UI_GET:'+path,flush=True); serial('ZORIX_UI_GET:'+path)
   p=(self.s.ui/urllib.parse.unquote(path).lstrip('/')).resolve()
   if not p.is_relative_to(self.s.ui) or not p.is_file():return self.send(404,{'error':'Not found'})
   return self.send(200,p.read_bytes(),mimetypes.guess_type(p)[0] or 'application/octet-stream')
  except (OSError,ValueError) as e:return self.send(400,{'error':str(e)[:200]})
 def do_POST(self):
  if not self.auth():
   path=urllib.parse.urlsplit(self.path).path
   serial('ZORIX_AUTH_FAIL:POST:'+path+':token=' + ('yes' if bool(self.headers.get('X-Zorix-Token')) else 'no'))
   return self.send(403,{'error':'Session authorization required'})
  try:
   length=int(self.headers.get('Content-Length','0'))
   if not 0<length<=256000:return self.send(413,{'error':'Request too large or empty'})
   if not self.headers.get('Content-Type','').startswith('application/json'):raise ValueError('JSON required')
   data=json.loads(self.rfile.read(length))
   if not isinstance(data,dict):raise ValueError('Expected an object')
   path=urllib.parse.urlsplit(self.path).path
   with self.s.lock:
    if path=='/api/client-error':
     kind=str(data.get('kind','client'))[:40]
     message=str(data.get('message','unknown'))[:400]
     stack=str(data.get('stack',''))[:1200]
     line='ZORIX_JS_ERROR:'+kind+':'+message
     print(line,flush=True); serial(line)
     if stack: print(stack,flush=True)
     return self.send(200,{'logged':True})
    if path=='/api/ready':
     render=str(data.get('render','unknown'))[:32]
     ready=pathlib.Path(os.environ.get('XDG_RUNTIME_DIR','/tmp'))/'zorix-glass-ready'
     ready.write_text(render+'\n')
     print('ZORIX_GLASS_READY:'+render,flush=True); serial('ZORIX_GLASS_READY:'+render)
     self.s.record('glass-ready')
     return self.send(200,{'ready':True})
    if path=='/api/heartbeat':
     beat=int(data.get('beat',0))
     render=str(data.get('render','unknown'))[:32]
     if beat>0:
      line='ZORIX_GLASS_HEARTBEAT:'+render+':'+str(beat)
      print(line,flush=True); serial(line)
     return self.send(200,{'alive':True})
    if path=='/api/settings':
     settings=self.s.settings()
     for k,v in data.items():
      if k not in DEFAULT:raise ValueError('Unknown setting')
      if type(v) is not type(DEFAULT[k]):raise ValueError('Invalid setting type')
      if k=='theme' and v not in ('aurora','dawn','midnight'):raise ValueError('Unknown theme')
      if k=='language' and v not in ('en','it'):raise ValueError('Unknown language')
      if k=='glassIntensity' and not 35<=v<=100:raise ValueError('Glass intensity out of range')
      settings[k]=v
     self.s.save(self.s.config/'glass.json',json.dumps(settings));self.s.record('settings-updated')
     return self.send(200,settings)
    if path=='/api/notes':
     text=data.get('text')
     if not isinstance(text,str) or len(text)>200000:raise ValueError('Note exceeds size limit')
     self.s.safe_path(str(self.s.notes));self.s.save(self.s.notes,text);self.s.record('note-saved');return self.send(200,{'saved':True})
    if path=='/api/tasks':
     action=data.get('action');tasks=self.s.task_list()
     if action=='add':
      text=data.get('text','')
      if not isinstance(text,str) or not text.strip() or len(text)>180:raise ValueError('Invalid task')
      tasks.append({'id':secrets.token_hex(6),'text':text.strip(),'done':False,'created':int(time.time())});tasks=tasks[-60:]
     elif action in ('toggle','delete'):
      ident=data.get('id')
      if not isinstance(ident,str):raise ValueError('Invalid task id')
      found=False;new=[]
      for item in tasks:
       if item.get('id')==ident:
        found=True
        if action=='delete':continue
        item=dict(item);item['done']=not bool(item.get('done'))
       new.append(item)
      if not found:raise ValueError('Task not found')
      tasks=new
     else:raise ValueError('Unknown task action')
     self.s.save(self.s.tasks,json.dumps(tasks,ensure_ascii=False));self.s.record('tasks-updated');return self.send(200,{'tasks':tasks})
    if path=='/api/mkdir':
     name=data.get('name',''); parent=self.s.safe_path(data.get('path',''))
     if not isinstance(name,str) or not name or len(name)>120 or '/' in name or '\\' in name or name in ('.','..'):raise ValueError('Invalid folder name')
     (parent/name).mkdir(mode=0o700);self.s.record('folder-created');return self.send(200,{'created':True})
    if path=='/api/launch':
     key=data.get('app')
     if key in APPS:
      argv=[p.replace('{home}',str(self.s.home)) for p in APPS[key]]; logkey=key
     elif isinstance(key,str) and key.startswith('desktop:'):
      self.s.desktop_apps(); desktop=self.s.desktop_map.get(key)
      if desktop is None:raise ValueError('Installed application is no longer available')
      if not shutil.which('gio'):return self.send(409,{'error':'Desktop launcher runtime is unavailable'})
      argv=['/usr/bin/gio','launch',str(desktop)]; logkey='desktop'
     else:raise ValueError('Application is not allowlisted')
     if self.s.testing:return self.send(200,{'testMode':True,'argv':argv})
     if not os.access(argv[0],os.X_OK):return self.send(409,{'error':'Application runtime not installed'})
     logfile=self.s.config/f'{logkey}.log'
     with open(logfile,'ab') as log:subprocess.Popen(argv,stdin=subprocess.DEVNULL,stdout=log,stderr=log,start_new_session=True)
     self.s.record('launched-'+logkey);return self.send(200,{'launched':key})
    if path=='/api/power':
     action=data.get('action')
     if action not in ('poweroff','reboot') or data.get('confirmation')!=action:raise ValueError('Explicit confirmation required')
     if self.s.testing:return self.send(200,{'testMode':True,'action':action})
     if not pathlib.Path('/etc/zorix-live').exists():return self.send(409,{'error':'Power action only available in Zorix Live'})
     subprocess.Popen(['/usr/bin/sudo','-n','/usr/bin/zorix-live-power',action],stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
     return self.send(200,{'requested':action})
   return self.send(404,{'error':'Unknown API'})
  except (OSError,ValueError,TypeError) as e:return self.send(400,{'error':str(e)[:200]})

def make_server(state,port=0):
 server=ThreadingHTTPServer(('127.0.0.1',port),Handler);server.state=state;server.daemon_threads=True;return server

def main():
 p=argparse.ArgumentParser();p.add_argument('--ui',type=pathlib.Path,default=pathlib.Path('/usr/share/zorix/glass'));p.add_argument('--core',type=pathlib.Path,default=pathlib.Path('/usr/bin/zorix-core'));p.add_argument('--port',type=int,default=0);p.add_argument('--no-browser',action='store_true');p.add_argument('--test-mode',action='store_true');p.add_argument('--session-file',type=pathlib.Path);a=p.parse_args()
 if os.getuid()==0 and not a.test_mode:sys.exit('Start the Zorix desktop as an ordinary user, not root.')
 state=State(pathlib.Path.home(),a.ui,a.core,a.test_mode);server=make_server(state,a.port)
 render=urllib.parse.quote(os.environ.get('ZORIX_RENDER_MODE','portable')); url=f'http://127.0.0.1:{server.server_port}/#token={state.token}&render={render}'
 if a.session_file:state.save(a.session_file,json.dumps({'url':url,'pid':os.getpid(),'port':server.server_port}))
 threading.Thread(target=server.serve_forever,daemon=True).start()
 child=None
 try:
  if not a.no_browser:
   chromium='/usr/lib/chromium/chromium' if os.access('/usr/lib/chromium/chromium',os.X_OK) else '/usr/bin/chromium'
   argv=[chromium,'--app='+url,'--class=ZorixGlass','--user-data-dir='+str(state.config/'glass-browser'),'--no-first-run','--disable-sync','--disable-extensions','--disable-background-networking','--disable-component-update','--ozone-platform=x11','--disable-dev-shm-usage','--no-default-browser-check','--password-store=basic','--start-maximized','--renderer-process-limit=2','--disk-cache-size=67108864','--media-cache-size=33554432']
   # Native Xorg can use Chromium's normal GPU auto-detection. Portable Xvfb
   # has no real GPU, so keep software rendering there to avoid probe stalls.
   if os.environ.get('ZORIX_RENDER_MODE')=='portable':
    argv += [
     '--disable-gpu',
     '--disable-software-rasterizer=false',
     '--disable-features=Vulkan,UseSkiaRenderer,Dawn,WebGPU,CanvasOopRasterization',
     '--disable-background-timer-throttling',
     '--disable-renderer-backgrounding',
     '--disable-backgrounding-occluded-windows'
    ]
   else:
    argv += ['--enable-gpu-rasterization']
   print('ZORIX_BROWSER_START:'+os.environ.get('ZORIX_RENDER_MODE','unknown')+':'+chromium,flush=True); serial('ZORIX_BROWSER_START:'+os.environ.get('ZORIX_RENDER_MODE','unknown')+':'+chromium)
   child=subprocess.Popen(argv)
   browser_rc=child.wait()
   print('ZORIX_BROWSER_EXIT:'+str(browser_rc),flush=True); serial('ZORIX_BROWSER_EXIT:'+str(browser_rc))
   if browser_rc!=0:
    raise SystemExit(browser_rc)
  else:
   while True:time.sleep(1)
 except KeyboardInterrupt:pass
 finally:
  server.shutdown()
  if child and child.poll() is None:child.terminate()
if __name__=='__main__':sys.exit(main() or 0)
