/* SPDX-License-Identifier: MIT */
(function(){
  'use strict';
  var params=new URLSearchParams(location.hash.slice(1));
  var token=params.get('token')||'';
  var renderMode=params.get('render')||'native';
  document.documentElement.setAttribute('data-render',renderMode);
  if(renderMode==='portable') document.documentElement.setAttribute('data-low-cost','true');
  function post(path,data){
    try{
      var x=new XMLHttpRequest();
      x.open('POST','/api/'+path,true);
      x.setRequestHeader('Content-Type','application/json');
      x.setRequestHeader('X-Zorix-Token',token);
      x.send(JSON.stringify(data||{}));
    }catch(e){}
  }
  window.addEventListener('error',function(e){
    post('client-error',{kind:'boot-window-error',message:String(e.message||'script error'),stack:''});
  });
  window.addEventListener('unhandledrejection',function(e){
    var r=e.reason||'promise rejection';
    post('client-error',{kind:'boot-rejection',message:String(r&&r.message?r.message:r),stack:String(r&&r.stack?r.stack:'').slice(0,1200)});
  });
  document.documentElement.setAttribute('data-js-engine','active');
  post('boot-probe',{ua:navigator.userAgent||'',readyState:document.readyState});
}());