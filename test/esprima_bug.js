console = {
log:print
}

(function() {

var i = 0;
var j = 8;

checkiandj: while (i < 4) {
  console.log("i: " + i);
  i += 1;

  checkj: while (j > 4) {
    console.log("j: "+ j);
    j -= 1;

    if ((j % 2) == 0)
      continue checkj;
    console.log(j + " is odd.");
  }
  console.log("i = " + i);
  console.log("j = " + j);
}
}());


(function() {

outer_block: {
  inner_block: {
    console.log('1');
    break outer_block; // breaks out of both inner_block and outer_block
    console.log(':-('); // skipped
  }
  console.log('2'); // skipped
}

}());

