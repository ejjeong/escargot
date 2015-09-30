var i = 0;
x: {
  i++;
  if (i < 3) {
    for(var a in [1,2,3,4,5]) {
      print(a);
      for(var b in [1,2,3,4,5,6,7,8,9,10]) {
        if(b % 2 == 0)
          break x; 
      }
    }
  }
}
