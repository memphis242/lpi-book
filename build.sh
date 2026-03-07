gcc -std=c23 -Og -g3 -Wall -Wextra -Wpedantic -pedantic-errors -c util.c -o util.o
ar rcs libutil.a util.o
gcc -std=c23 -Og -g3 -Wall -Wextra -Wpedantic -pedantic-errors -Wno-unused-parameter -Wno-unused-variable -o ch6-processes-6-1 ch6-processes-6-1.c -L. -lutil
