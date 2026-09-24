// SPDX-License-Identifier: MIT
use std::fs;
use std::thread;

fn read(path: &str) -> String {
    fs::read_to_string(path).unwrap_or_default()
}
fn json_escape(s: &str) -> String {
    s.replace('\\', "\\\\").replace('"', "\\"").replace('\n', "\\n")
}
fn cpu_model(cpuinfo: &str) -> String {
    for line in cpuinfo.lines() {
        if let Some((k,v)) = line.split_once(':') {
            if k.trim() == "model name" { return v.trim().to_string(); }
        }
    }
    "unknown".to_string()
}
fn mem_kib(meminfo: &str, key: &str) -> u64 {
    for line in meminfo.lines() {
        if let Some((k,v)) = line.split_once(':') {
            if k.trim() == key {
                return v.split_whitespace().next().and_then(|x| x.parse().ok()).unwrap_or(0);
            }
        }
    }
    0
}
fn main() {
    let kernel = read("/proc/sys/kernel/osrelease").trim().to_string();
    let cpuinfo = read("/proc/cpuinfo");
    let meminfo = read("/proc/meminfo");
    let uptime = read("/proc/uptime").split_whitespace().next().unwrap_or("0").to_string();
    let load = read("/proc/loadavg");
    let load3: Vec<&str> = load.split_whitespace().take(3).collect();
    let cpus = thread::available_parallelism().map(|x| x.get()).unwrap_or(1);
    println!("{{\"system\":\"Zorix OS\",\"version\":\"0.7.0\",\"kernel\":\"{}\",\"cpuModel\":\"{}\",\"logicalCPUs\":{},\"memoryTotalKiB\":{},\"memoryAvailableKiB\":{},\"uptimeSeconds\":{},\"loadAverage\":[{}]}}",
        json_escape(&kernel), json_escape(&cpu_model(&cpuinfo)), cpus,
        mem_kib(&meminfo, "MemTotal"), mem_kib(&meminfo, "MemAvailable"), uptime,
        load3.join(","));
}