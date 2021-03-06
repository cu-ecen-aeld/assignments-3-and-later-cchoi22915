#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/
    //Check for NULL
    if(cmd == NULL)
    return false;

    //Call system function with cmd in parameter
    int syscmd = system(cmd);
    
    //Check return from system call
    WIFEXITED(syscmd);

    if (syscmd == true)
    	return true;
    else
    	return false;

}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/
    
    //save status
    int status;
    
    //fork
    pid_t pid = fork();

    //check for fork error
    if (pid == -1) {
        perror("ERROR: Failed to fork()...");
        return false;
    }
    //if pid equals zero..
    else if (pid == 0) 
    {
        //execv and perror then exit
        execv(command[0], command);
        perror("ERROR: exec() failure...");
        exit(-1);
    }
    //for non-zero value
    else {
        //check for waitpid error
        if (waitpid(pid, &status, 0) == -1) 
        {
            perror("ERROR: waitpid failure...");
            return false;
        }
        //ensure child process closure
        else if (WIFEXITED(status) == true && WEXITSTATUS(status) != 0) 
        {
            return false;
        }
    }

    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/

    int status;
    pid_t pid;

    int fd = open(outputfile, O_WRONLY|O_CREAT, 0644);

    if (fd == -1) {
        perror("ERROR: Failed to open file...");
        return false;
    }

    pid = fork();

    if (pid == -1) 
    {
    	perror("ERROR: Failed to fork...");
        return false;
    }
    else if (pid == 0) 
    {

        if (dup2(fd, STDOUT_FILENO) < 0) 
        {
            perror("ERROR: Failed to call dup2...");
            return false;
        }

        close(fd);

        execv(command[0], command);

        perror("ERROR: Failure in exec...");
        exit(EXIT_FAILURE);
    }
    else 
    {
        close(fd);

        if (waitpid(pid, &status, 0) == -1) 
        {
            perror("ERROR: Failed in status update in waitpid...");
            return false;
        }
        else if ((WIFEXITED(status) == true) && (WEXITSTATUS(status) != 0)) 
        {
            return false;
        }
    }

    va_end(args);
    
    return true;
}
