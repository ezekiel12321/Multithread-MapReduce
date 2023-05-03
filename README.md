This program is an efficient and powerful implementation of the classic word count problem, leveraging multithreading to significantly speed up the processing of large text files. The program is written in C and is designed to impress interviewers with its subtle complexity and performance.

Features
Multithreaded processing using a custom MapReduce framework.
Highly efficient memory management and resource allocation.
Robust error handling and input validation.
Processes multiple input files concurrently.
Designed with code reusability in mind, using modular functions.
Usage
makefile
Copy code
Usage: wordcount num_mapper num_reducer file...
num_mapper: The number of mapper threads to use.
num_reducer: The number of reducer threads to use.
file...: One or more input files to process.
How It Works
The program reads the input files line by line and adds them to the input list.
The map_reduce function is called with the user-defined number of mapper and reducer threads.
Mapper threads tokenize the input lines into words and add them to the output list as key-value pairs, with the word as the key and "1" as the value.
Reducer threads aggregate the word counts, summing the values for each unique word.
The final word count is printed to the standard output.
Key Functions
toLowerStr: Helper function that converts a string to lowercase.
mapper: Tokenizes a line into words and adds key-value pairs to the output list.
reducer: Aggregates the word counts by summing the values for each unique word.
Impressive Aspects
Utilizes the power of multithreading to maximize efficiency and performance.
Carefully handles memory management and resource allocation.
Robust error handling ensures program stability.
Flexible and extensible design allows for easy modification and customization.
Elegant use of C language features and standard library functions.

Usage: wordcount num_mapper num_reducer file...
num_mapper: The number of mapper threads to use.
num_reducer: The number of reducer threads to use.
file...: One or more input files to process.

How It Works
The program reads the input files line by line and adds them to the input list.
The map_reduce function is called with the user-defined number of mapper and reducer threads.
Mapper threads tokenize the input lines into words and add them to the output list as key-value pairs, with the word as the key and "1" as the value.
Reducer threads aggregate the word counts, summing the values for each unique word.
The final word count is printed to the standard output.
Key Functions
toLowerStr: Helper function that converts a string to lowercase.
mapper: Tokenizes a line into words and adds key-value pairs to the output list.
reducer: Aggregates the word counts by summing the values for each unique word.
Impressive Aspects
Utilizes the power of multithreading to maximize efficiency and performance.
Carefully handles memory management and resource allocation.
Robust error handling ensures program stability.
Flexible and extensible design allows for easy modification and customization.
Elegant use of C language features and standard library functions.

