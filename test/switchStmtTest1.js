for (var i = 0; i < 4; i++){
    switch(i) {
        case 0:
            continue;
        case 1:
        case 2:
            print(i);
        default:
            break;
    }

            switch (j) {
                case 5:
                case 10:
                    continue;
            }
}
var j = 10;
for (var i = 0; i < 4; i++){
    switch(i) {
        case 0:
        case 1:
            switch (j) {
                case 5:
                case 10:
                    continue;
            }
        case 2:
            print(i);
        default:
            break;
    }
}
