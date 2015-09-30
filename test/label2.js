var i = 1;

function f() {
  i--;
  print('i : ' + i);
  _1: while(true) {
    if(i < 0) {
      break _1;
    }else {
      f();
      continue _1;
    }
  }
}

f();
