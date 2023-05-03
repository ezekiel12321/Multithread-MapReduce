make clean
make
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./word-count 22 22  file1.txt file2.txt 