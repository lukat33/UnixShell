#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

/************************************
*         GLOBAL VARIABLES
************************************/

char *tokens[255];           // array of strings, tokens
int token_count = 0;         // number of tokens
char *terminal_name = "mysh";
char *current_dir;
int exit_status = 0;
int current_in;              // stdin = 0
int current_out;             // stdout = 1


/************************************
*           ALL FUNCTIONS
************************************/

// Create tokens from an input line
void tokenize(char *line)
{
    // Reset tokens array
    memset(tokens, 0, 255);
    // Reset tokens number to 0
    token_count = 0;
    // Remove line break char
    int line_len = strlen(line);

    char token[255];
    int token_len = 0;
    int quote_mode = 0;
    int prvi_znak = 0;

    for (int i=0; i<line_len; i++)
    {
        char znak = line[i];
        int char_empty = isspace(znak);
        // Check for only white-space chars
        if (prvi_znak == 0 && char_empty)
            continue;
        else
            prvi_znak = 1;

        // Begin quote_mode "...
        if (znak == '"' && quote_mode == 0)
        {
            quote_mode = 1;
            continue;
        }
        // Turn OFF quote_mode
        else if (znak == '"' && quote_mode == 1)
        {
            quote_mode = 0;
            i++;
        }

        // Symbol is not a white-space char
        if ((char_empty == 0 || quote_mode == 1) && znak != '"')
        {
            token[token_len] = znak;
            token_len++;

        }

        // symbol = white-space, we are not in "..."
        // or
        // end quote mode ..."
        if (char_empty != 0 && quote_mode == 0 ||
            znak == '"' && quote_mode == 0)
        {
            // Null terminate token so we can print it
            token[token_len] = '\0';
            tokens[token_count] = strdup(token);
            token_count++;
            memset(token, 0, 255);
            token_len = 0;
        }
    }
}

void print_tokens()
{ 
    for (int i=0; i<token_count; i++)
    {
        printf("%s ", tokens[i]);
    }
    printf("\n");
    fflush(stdout);
}

void action_name()
{
    // New name was given
    if (token_count > 1)
        terminal_name = strdup(tokens[1]);
    else
        printf("%s\n", terminal_name);

    fflush(stdout);
    exit_status = 0;
}

void action_help()
{
    printf(
        "  ------------------ HELP --------------------\n\n"
        "   SIMPLE COMMANDS:\n"
        "   - name\n"
        "   - help\n"
        "   - status\n"
        "   - exit \n"
        "   - print\n"
        "   - echo\n"
        "   - pid\n"
        "   - ppid\n"
        "\n"
        "   DIRECTORY MANIPULATION COMMANDS:\n"
        "   - dirchange\n"
        "   - dirwhere\n"
        "   - dirmake\n"
        "   - dirremove\n"
        "   - dirlist\n"
        "\n"
        "   FILE MANIPULATION COMMANDS:\n"
        "   - linkhard\n"
        "   - linksoft\n"
        "   - linkread\n"
        "   - linklist\n"
        "   - unlink\n"
        "   - rename\n"
        "   - cpcat\n"
        "\n"
        "   PIPELINE:\n"
        "   Example:\n"
        "   - pipes \"cat /etc/passwd\" \"cut -d: -f7\" \"sort\" \"uniq -c\"\n\n"
        "\n"
        "  --------------------------------------------\n"
        );
    fflush(stdout);
    exit_status = 0;
}

void action_status()
{
    printf("%d\n", exit_status);
    fflush(stdout);
    exit_status = 0;
}

void action_print()
{
    for (int i=1; i<token_count; i++)
    {
        if (i == token_count -1)
            printf("%s", tokens[i]);
        else
            printf("%s ", tokens[i]);

        fflush(stdout);
    }
    exit_status = 0;
}

void action_echo()
{
fflush(stdout);
    for (int i=1; i<token_count; i++)
    {
        if (i == token_count -1)
            printf("%s", tokens[i]);
        else
            printf("%s ", tokens[i]);

    }
    printf("\n");
    fflush(stdout);
    exit_status = 0;
}

void action_dirchange()
{
    // We use default path if path was not given
    char *path = "/";
    if (token_count > 1)
        path = tokens[1];

    // chdir returns 0 on success
    int error = chdir(path);            

    // There was no error
    if (error == 0)
    {
        // memset(current_dir, 0, 255);
        getcwd(current_dir, 255);
        // printf("%d\n", error);
    }
    else
    {
        exit_status = error;
        perror("dirchange");
    } 
}

// int mkdir(char *pathname, mode_t mode);
void action_dirmake()
{
    // If mkdir fails it returns -1
    if (mkdir(tokens[1], S_IRWXU) == -1)
    {
        exit_status = errno;
        perror("dirmake");
    }
}

// int rmdir(char *path);
void action_dirremove()
{
    // If rmdir fails it returns -1
    if (rmdir(tokens[1]) == -1)
    {
        exit_status = errno;
        perror("dirremove");
    }
}

void action_dirlist()
{
    // If directory is not given, we
    // use current working directory
    char *dirname = ".";
    DIR *dirp;
    struct dirent *entry;
    // Directory is given
    if (token_count > 1)                 
        dirname = tokens[1];

    dirp = opendir(dirname);
    if (dirp == NULL)
    {
        exit_status = errno;
        perror("dirlist");
    }
    else    
    {   
        // readdir() returns a pointer to a structure
        // representing the directory entry
        while(entry = readdir(dirp))
        {
            if(entry->d_name[0])
                printf("%s  ", entry->d_name);
        }
        printf("\n");
        fflush(stdout);
        closedir(dirp);
    }
}

// int rename(char *old_filename, char *new_filename)
void action_rename()
{
    // if rename fails it returns -1
    if (rename(tokens[1], tokens[2]) == -1)         
    {
        exit_status = errno;
        perror("rename");
    }
}

// int link(char *oldpath, char *newpath);
void action_linkhard()
{
    if (link(tokens[1], tokens[2]) == -1)
    {
        exit_status = errno;
        perror("linkhard");
    }
}
// int symlink(char *path1, char *path2);
void action_linksoft()
{
    if (symlink(tokens[1], tokens[2]) == -1) 
    {
        exit_status = errno;
        perror("linksoft");
    }
}

// int symlink(char *path1, char *path2);
void action_linkread()                  
{
    char buf[255];

    if (readlink(tokens[1], buf, 255) == -1) 
    {
        exit_status = errno;
        perror("linkread");
    }
    else
    {
        printf("%s\n", buf);
        fflush(stdout);
    }
}
void action_linklist()
{
    // If directory is not given, we
    // use current working directory
    char *dirname = ".";                
    DIR *dirp;
    struct dirent *entry;
    struct stat fileStat;
    int inode_file;

    int fd = open(tokens[1], O_RDONLY);
    if (fd == -1)
    {
        exit_status = errno;
        perror("linklist");
    }
    if (fstat(fd, &fileStat) < 0)
    {
        exit_status = errno;
        perror("linklist");
    }
    inode_file = fileStat.st_ino;
    
    dirp = opendir(dirname);
    if (dirp == NULL){
        exit_status = errno;
        perror("linklist");
    }
    else    
    {  
        // readdir() returns a pointer to a structure
        // representing the directory entry
        while(entry = readdir(dirp))
        {
            if ((int)entry->d_ino == (int)inode_file)
                printf("%s  ", entry->d_name);
        }

        printf("\n");
        fflush(stdout);
        closedir(dirp);
        close(fd);
    }
}

// int unlink(const char *path);
void action_unlink()
{
    if (unlink(tokens[1]) == -1)
    {
        exit_status = errno;
        perror("unlink");
    }
}

void cpcat(int fd, int out)
{
    char buf[255];
    int bytes_read, bytes_write, err, nbytes = sizeof(buf);

    while (1)
    {
        bytes_read = read(fd, buf, nbytes);
        
        if (bytes_read == -1) 
        {
            exit_status = errno;
            perror("cpcat"); 
        }
        
        // Is empty
        if (bytes_read == 0)            
            break;
        
        err = write(out, &buf, bytes_read); 
        if (err == -1)
        {
            exit_status = errno;
            perror("cpcat"); 
        }
    }         
}

void action_cpcat()
{
    int in = 0; 
    int out = 1;
    
    // stdin to stdout
    if (token_count == 1)
    {             
        in = 0;
        out = 1;
    }
    // file to stdout, write file
    else if (token_count == 2)          
    {   
        in = open(tokens[1], O_RDONLY); 
        
        if (in == -1)
        {
            exit_status = errno;
            perror("cpcat"); 
        }   
    }
    // stdin to file or fileA to fileB
    else if (token_count == 3)          
    {
        if (tokens[1][0] == '-')
            in = 0;
        else 
        {
            in = open(tokens[1], O_RDONLY);
                
            if (in == -1)
            {
                exit_status = errno;
                perror("cpcat"); 
            }
        }
        
        out = open(tokens[2], O_CREAT|O_WRONLY|O_TRUNC,S_IWRITE|S_IREAD); 
    
        if(out == -1)
        {
            exit_status = errno;
            perror("cpcat"); 
        }
    }
    cpcat(in, out);
}

void zunanji_ukaz()
{
    int background = 0;
    // Execution in background  &
    if (tokens[token_count-1][0] == '&')    
    {   
        background = 1;
        token_count--;
        tokens[token_count] = '\0';
        // Copy without & in the end
        memcpy(&tokens, &tokens, sizeof(tokens)-sizeof(tokens[token_count]));
    }
    pid_t pid = fork();
    if (pid < 0)
        exit(0);

    // CHILD, execute command and exit
    if (pid == 0)                       
    {  
        tokens[token_count] = NULL;

        execvp(tokens[0], tokens);
        exit(0);
    }
    // PARENT, wait if process is not in background
    if (pid > 0 && !background)         
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

// Remove zombies
void remove_zombie(int signum) {        
    int status;
    waitpid(-1, &status, WNOHANG);
}

void preusmeritev()
{
    fflush(stdout);

    // Redirect stdout % who > names : Redirect standard output to a file named names
    if (tokens[token_count-1][0] == '>')
    {
        char name[strlen(tokens[token_count-1])];
        strcpy(name, tokens[token_count-1]+1);
        name[strlen(name)] = '\0';
        
        token_count--;
        tokens[token_count] = '\0';
        memcpy(&tokens, &tokens, sizeof(tokens)-sizeof(tokens[token_count]));

        int fdout = open(name, O_CREAT|O_WRONLY,S_IWRITE|S_IREAD);

        if (fdout == -1)
            exit_status = errno;

        dup2(fdout, STDOUT_FILENO);
        close(fdout);
    }

    // Redirect stdin
    if (tokens[token_count-1][0] == '<')       
    {
        char name[255];
        strncpy(name, tokens[token_count-1] + 1, strlen(tokens[token_count-1]));
        token_count--;

        int fd = open(name, O_RDONLY);

        if (fd == -1)
            exit_status = errno;

        dup2(fd, STDIN_FILENO);
        close(fd);
    }
}
/************************************
*         DRIVER FUNCTION
************************************/
// Determine action and call given function
void driver_function()                 
{
    char *action = strdup(tokens[0]);

    if (strcmp(action, "name") == 0) action_name();
    else if (strcmp(action, "help") == 0) action_help();
    else if (strcmp(action, "status") == 0) action_status();
    else if (strcmp(action, "exit") == 0) exit(atoi(tokens[1]));
    else if (strcmp(action, "print") == 0) action_print();
    else if (strcmp(action, "echo") == 0) action_echo();
    else if (strcmp(action, "pid") == 0) printf("%d\n", getpid());
    else if (strcmp(action, "ppid") == 0) printf("%d\n", getppid());
    else if (strcmp(action, "dirchange") == 0) action_dirchange();
    else if (strcmp(action, "dirwhere") == 0) printf("%s\n", current_dir);
    else if (strcmp(action, "dirmake") == 0) action_dirmake();
    else if (strcmp(action, "dirremove") == 0) action_dirremove();
    else if (strcmp(action, "dirlist") == 0) action_dirlist();
    else if (strcmp(action, "rename") == 0) action_rename();
    else if (strcmp(action, "linkhard") == 0) action_linkhard();
    else if (strcmp(action, "linksoft") == 0) action_linksoft();
    else if (strcmp(action, "linkread") == 0) action_linkread();
    else if (strcmp(action, "linklist") == 0) action_linklist();
    else if (strcmp(action, "unlink") == 0) action_unlink();
    else if (strcmp(action, "cpcat") == 0) action_cpcat();
    // Shell Builtin commands
    else zunanji_ukaz();
}  

/************************************
*              MAIN
************************************/
int main(int argc, char* args[])
{
    signal(SIGCHLD, remove_zombie);
    current_dir = malloc(sizeof(255));
    getcwd(current_dir, 255);
    int is_terminal = isatty(0);

    char line[255];
    if(is_terminal)
    {
        printf("%s> ", terminal_name);
        fflush(stdout);
    }
    
    // ctrl+d or ctrl+z or ctrl+c to terminate
    while(fgets (line, 255, stdin)) 
    {   
        tokenize(line);
        current_in = dup(STDIN_FILENO);
        current_out = dup(STDOUT_FILENO);

        if (token_count > 0 && tokens[0][0] != '#')
        {
            // Determine redirections, depending on < or >
            preusmeritev();
            driver_function();
            fflush(stdout);

            // Restore descriptors
            dup2(current_in, STDIN_FILENO);    
            dup2(current_out, STDOUT_FILENO);
        }

        if(is_terminal)
        {
            printf("%s> ", terminal_name);
            fflush(stdout);
        }

        fflush(stdout);
        memset(line, 0, 255);
    }
    free(current_dir);
    return 0;
}