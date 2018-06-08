main() {
  setuid(500);
  setreuid(500,500);
  seteuid(500);
  setfsuid(500);
  system("killcrafty.sh");
  exit(0);
}
