Source code manager like Git.

The main goal of this project is to show how a source code manager 
works in as little code as possible. 

The others goals are

1. Keep the code very modular. 
2. Avoid Memory leaks/corruption during development phase by using valgrind tool. 
3. Keep the code very simple and easy to understand. 

Currently supported commands are 
1. init.    => Initialize a new repo.
2. checkout => Checkout files or a branch.
3. branch   => Create a new branch.
4. status   => Show thats files that have changed.
5. ls       => Show the list of files under revision control.
6. commit   => Commit the changes.
7. rm       => delete a files from revision control.
8. sha      => Print the SHA id of the file.
