#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

void print(char *str) { // legit just prints the given string. Works for both variables and straight up text.
    int len = strlen(str);
    write(1, str, len);
}

void printn(char *str) { // exact same but with a newline so i dont have to
    int len = strlen(str);
    write(1, str, len);
    write(1, "\n", 1);
}

char *input() {
    //char *str = (char*)malloc(1024);
    char *str = malloc(1024);
    read(0, str, 1024); // create a big ole buffer
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == '\n') { // if they hit the enter key turn it into a null byte rather then a new line
            str[i] = '\0';
            break;
        }
    }
    return str;
}

char **split(char *str, int *count) {
    char **splitted = malloc(100 * sizeof(char *)); //hopefully 100 words is the max
    *count = 0;
    int index = 0;

    while (str[index] != '\0') { // while we havent reached the end of the string
        while(str[index] == ' ') { // if there is more then one space between words index until it isn't
            index++;
        }
        if (str[index] == '\0') {//if we reached the end of the string break the loop and return the array
            break;
        }

        // WARNING THIS IS REALLY GROSS LOOKING
        // I figured that since i already have a list of arguements going i might as well split at operators too
        if (str[index] == '>') { // nif the index is an output redirect check the next index for the same char
            if (str[index + 1] == '>') {
                char *substr = malloc(3); //malloc and assign the values accordingly
                substr[0] = '>';
                substr[1] = '>';
                substr[2] = '\0';
                splitted[*count] = substr;
                (*count)++;
                index += 2;
                continue;
            } else {
                char *substr = malloc(2); //only allot space for two chars, the null byte and the lonesome '>'
                substr[0] = '>';
                substr[1] = '\0';
                splitted[*count] = substr;
                (*count)++;
                index++;
                continue;
            }
        }
        // repeat that for the input redirect
        if (str[index] == '<') {
            char *substr = malloc(2);
            substr[0] = '<';
            substr[1] = '\0';
            splitted[*count] = substr;
            (*count)++;
            index++;
            continue;
        }

        int start = index; //finds the start of the word
        //iterates through until the end or a space
        while(str[index] != ' ' && str[index] != '\0' && str[index] != '>' && str[index] != '<') {
            index++;
        }
        int end = index;// saves the ending index
        int len = end - start; // creates a variable sotring the word length
        char *word = malloc(len + 1);

        for (int j = 0; j < len; j++) { //while j is less then the length of the word
            word[j] = str[start + j]; //assign the char at the index
        }
        word[len] = '\0'; //put the nullbyte back on in the very last position in the string
        splitted[*count] = word; // add the new word into the array
        (*count)++;
    }
    return splitted;
}

void userInput() {
    char directory[1024]; //set a big ole buffer
    if (getcwd(directory, 1024) != NULL) { // if you successfully yoink the path
        print(directory); // use my python ish print function
        print(" $");
    }
    else {
        char *errMsg = "getcwd failed";
        printn(errMsg);
    }
}

// prints the working directory
void pwd() {
    char path[1024];//create a buffer for the path
    if (getcwd(path, 1024) != NULL) {
        print("current directory: ");
        printn(path);
    } else {
        printn("getcwd failed");
    }
}

void debugArgs(char **args) {
    //this is slightly different but I have the operators listed within this as well. It was just more beneficial for me to see where they are located and if my split fuction caught them properly
    //I get rid of the operators and set them to null with the runExternal function so it technically doesn't harm my code in any way. Just ended up being a personal preference
    int i = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (i == 0) {
            print("command: ");
            printn(args[i]);
        }
        else if (strcmp(args[i], ">>") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0) {
            print("operator:");
            printn(args[i]);
        }
        else {
            print("arg: ");
            printn(args[i]);
        }
    }
}

void cd(char **args) {
    if (args[1] == NULL) { // if they havent put in a directory target directory
        print("invalid input");
    }
    else {
        char *dir = args[1];
        if (chdir(dir) == -1) {
            print("could not cd into directory: ");
            printn(dir);
        } else {
            print("success!! ");
            pwd();
            printn(" ");
        }
    }
}

void runExternal(char **args) { // this is gonna run for and exec and all that jazz
    pid_t pid = fork();

    if (pid == 0) {
        int append = 0; int write = 0; int read = 0;
        int OUTredirectIndex = -1; // if there isn't a redirect this shouldn't make a difference
        int INredirectIndex = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], ">>") == 0) {
                if (!append) {
                    append = 1;
                    OUTredirectIndex = i;
                } else {
                    printn("error: there are too many conditionals");
                }
            }else if (strcmp(args[i], ">") == 0) {
                if (!write) {
                    write = 1;
                    OUTredirectIndex = i;
                }else {
                    printn("error: there are too many conditionals");
                }
            }else if (strcmp(args[i], "<") == 0) {
                if (!read) {
                    read = 1;
                    INredirectIndex = i;
                }else {
                    printn("error: there are too many conditionals");
                }
            }
        }
        int f; // initialize the file desriptor
        if (append) {
            close(STDOUT_FILENO);
            if (open(args[(OUTredirectIndex + 1)], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU) < 0) {
                printn(strerror(errno));
                exit(1);
            }
            // printn("in append if block of runExternal");
            args[OUTredirectIndex] = NULL; // remove the operator from the arguement list
            args[OUTredirectIndex + 1] = NULL;

        } if (write) {
            close(STDOUT_FILENO);
            if (open(args[(OUTredirectIndex + 1)], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU) < 0) {
                printn(strerror(errno));
                exit(1);
            }
            args[OUTredirectIndex] = NULL;
            args[OUTredirectIndex + 1] = NULL;

        }if (read) { // Input redirection handler, closes stdin rather then stdout
            close(STDIN_FILENO);
            int fileDescriptor = open(args[(INredirectIndex + 1)], O_RDONLY);
            if (fileDescriptor < 0) {
                printn(strerror(errno));
                exit(1);
            }
            args[INredirectIndex] = NULL;
            args[INredirectIndex + 1] = NULL;
        }
        if (execvp(args[0], args) < 0){
            printn(strerror(errno));
        }
        exit(1);
    } else if (pid > 0) {
        waitpid(pid,NULL,0);
    } else {
        printn("something funky happened idk man");
        exit(1);
    }
}



// Simply makes main easier to look at. No other reason for this. returns true if you are exiting the program everything else returns false
bool menu(char **args, int count) {
    if (count == 0) {
        print("no input given");
    } else if (strcmp(args[0], "exit") == 0){
        printn("exiting the program.....");
        return true;

    } else if (strcmp(args[0], "pwd")==0) {
        pwd();
        return false;
    } else if (strcmp(args[0], "debugargs")==0) {
        debugArgs(args);
        return false;
    } else if(strcmp(args[0], "cd") == 0) {
        cd(args);
        return false;
    } else {
        runExternal(args);
        return false;
    }
}

int main() {
    bool done = false; // initialize done value
    while(!done) {
        // while we aren't done running the program
        userInput();
        char *command = input();

        int count; // initialize the count variable needed for split
        char **args = split(command, &count); // create the argument array based on the string given
        done = menu(args, count); // plug the array into our menu func.

        for (int i = 0; i < count; i++) { // go through and free all elements in the list (malloc was used for each word)
            free(args[i]);
        }
        free(args); // free up both command and args
        free(command);
    }
    return 0;
}