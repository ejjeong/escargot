import sys;

savedNames = ['3d-cube', '3d-raytrace', 'base64', 'crypto-aes', 'crypto-md5', 'crypto-sha1', 'date-format-tofte', 'date-format-xparb', 'n-body', 'regex-dna', 'tagcloud', 'towers.c', 'container.cpp', 'dry.c', 'n-body.c', 'quicksort.c', 'gcc-loops.cpp', 'hash-map', 'float-mm.c', 'bigfib.cpp', 'code-multi-load', 'richards', 'delta-blue', 'crypto', 'proto-raytracer', 'earley-boyer', 'regexp-2010', 'splay', 'splay-latency', 'navier-stokes', 'pdfjs', 'mandreel', 'mandreel-latency', 'gbemu', 'code-first-load', 'box2d', 'zlib', 'typescript', 'cdjs'];
        
def print_formatted(result):
    i = 0;
    for i in range(0, 39):
        print(result[i][0] + ' : ' + str(result[i][1]));

# argv[0] : res file name
def main(argv):
    resfile = open(argv[0], 'r');
    li = [[0 for i in range(39)] for j in range(1)];
    result = [[0 for i in range(2)] for j in range(39)];
    i = 0;

    while True:
        line = resfile.readline();
        if not line:
            break;
        if(line == '\n'):
            line = resfile.readline();

        benchmarkName = line.split(':')[0][0:-1];
        if(benchmarkName == savedNames[i]):
            result[i][0] = benchmarkName;
            result[i][1] = int(float(line.split(':')[-1][1:]));
        else:
            result[i][0] = savedNames[i];
            result[i][1] = "NaN";

        i+=1;
    print_formatted(result);

    resfile.close();

if __name__ == "__main__":
    main(sys.argv[1:]);

