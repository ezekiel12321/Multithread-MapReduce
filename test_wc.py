import random
import os
from collections import defaultdict

NUMBER_OF_FILES = 30
NUMBER_OF_TRIALS = 30
MAX_FILE_LEN = 10000
test_files = [f"test_files/{i}" for i in range(NUMBER_OF_FILES)]
delims = {c for c in "\t\r\n !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"}

if not os.path.isdir('out'):
    os.mkdir('out')
if not os.path.isdir('test_files'):
    os.mkdir('test_files')

# test empty file
os.system("touch test_files/empty_file")
os.system('valgrind --log-file="out/val" --leak-check=full ./word-count 13 27 test_files/empty_file > out/out_wc')
with open(f"out/val") as f:
    l = f.readlines()
    if "All heap blocks were freed -- no leaks are possible" not in l[11] or "ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)" not in l[14]:
        print(f"empty file shows not all freed or errors, check out/val")
        exit()
with open(f"out/out_wc") as f:
    assert f.read() == ""

# helper function to emulate word-count.c
def splitter(s):
    buildup = ""
    for char in s+" ":
        if char in delims and buildup:
            yield ''.join([c.lower() if 'A'<=c<='Z' else c for c in buildup]) # closer to C's tolower() than str.lower()
        buildup = buildup + char if char not in delims else ""

# make random files
for i in range(NUMBER_OF_FILES):
    with open(f"test_files/{i}", "w") as f:
        num_chars = random.randint(0, MAX_FILE_LEN)
        for j in range(num_chars):
            f.write(chr(random.randint(1, 255)))
            # f.write(chr(random.randint(1, 2**15)))

# run random trials
for i in range(NUMBER_OF_TRIALS):
    num_files = random.randint(1, 5)
    files_list = random.choices(test_files, k=num_files)

    # get correct word counnt
    d_correct = defaultdict(int)
    for file in files_list:
        with open(file, "r") as data:
            for word in splitter(data.read()):
                d_correct[word] += 1

    # get student word count
    os.system(f'valgrind --log-file="out/val_{i}" --leak-check=full ./word-count {random.randint(1,100)} {random.randint(1,100)} {" ".join(files_list)} > out/out_wc{i}\n')
    d_student = defaultdict(int)
    with open(f"out/out_wc{i}", "r") as f:
        for line in f:
            l = line.split(',')
            d_student[l[0]] = int(l[1])
    
    # compare to correct
    if d_student != d_correct:
        print(f"trial {i} does not match")
        print("correct", d_correct)
        print("stud", d_student)
        print("read files:", " ".join(files_list))
        exit()

    print(f"trial {i} success")

# check valgrind
for i in range(NUMBER_OF_TRIALS):
    with open(f"out/val_{i}") as f:
        l = f.readlines()
        if "All heap blocks were freed -- no leaks are possible" not in l[11]:
            print(f"valgrind of file out/val_{i} (from trial {i}) shows not all memory freed")
        if "ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)" not in l[14]:
            print(f"valgrind of file out/val_{i} (from trial {i}) shows errors")
print("valgrind passed")
print("all tests passed") 