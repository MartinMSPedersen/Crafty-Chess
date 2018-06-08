main() {
  setuid(18);
  system("cd /home/hyatt/ics;nohup ics>& ics.log &");
  exit(0);
}
