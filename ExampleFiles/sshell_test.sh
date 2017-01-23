#!/bin/bash
# Script for simple test cases for sshell
# Usage is just ./sshell_test.sh
# Make sure you have your sshell executable in your current directory
# And that you don't have any directories/files named:
# sshell_test_dir, out_test, err_test in your current directory

# Testing directories and files
TDIR=sshell_test_dir
OUTFILE=out_test
ERRFILE=err_test

# For testing later
path=$(pwd)

# counters 
correct=0
total=12

# Test correct exiting
exit_test(){
  #echo -e "exit\n" | ../sshell 1> ../out_test 2> ../err_test
  echo -e "exit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  if [ $test_str == "Bye..." ]; then
    let "correct++"
    echo -e "exit test -- PASS\n"
  else
    echo -e "exit test -- FAIL\n"
  fi

  # clean up
  rm $OUTFILE
  rm $ERRFILE
}

# Tests for correct mkdir output
mkdir_test(){
  echo -e "mkdir test_d\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '1q;d' $ERRFILE)
  if [ "$test_str" == "+ completed 'mkdir test_d' [0]" ]; then
    let "correct"++
    echo -e "mkdir test -- output only, not functionality -- PASS\n"
  else
    echo -e "mkdir test -- output only, not functionality -- FAIL\n"
  fi

  # clean up
  rm $OUTFILE
  rm $ERRFILE
  rm -rf test_d
}

# cd and pwd test
cd_and_pwd_test(){
  echo -e "mkdir test_d\ncd test_d\npwd\nexit\n" |& ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '1q;d' $ERRFILE) 
  test_str2=$(sed '4q;d' $OUTFILE)

  if [ "$test_str" == "+ completed 'mkdir test_d' [0]" ] &&
     [ "$test_str2" == "$path/$TDIR/test_d" ]; then
     let "correct"++
     echo -e "cd and pwd -- PASS\n"
  else
     echo -e "cd and pwd -- FAIL\n"
  fi

  # for checking if pwd was a syscall, outfile should be empty if syscall
  strace -f sh -c 'echo -e "pwd\nexit\n" | ../sshell' | grep "usr/bin/pwd" > $OUTFILE
  test_str=$(sed '1q;d' $OUTFILE)
  if [ "$test_str" == "" ]; then
     let "correct"++
     echo -e "pwd is syscall check -- PASS\n"
  else
     echo -e "pwd is syscall check -- FAIL\n"
  fi


  cd $path/$TDIR
  rm -rf test_d
  rm $OUTFILE
  rm $ERRFILE

}

# > redirect test -- requires cat and echo
redirect_out_test(){
  echo -e "echo hello > t\ncat t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '3q;d' $OUTFILE)

  if [ "$test_str" == "hello" ]; then
    let "correct"++
    echo -e "> redirect test -- PASS\n"
  else
    echo -e "> redirect test -- FAIL\n"
  fi

  rm $OUTFILE
  rm $ERRFILE
  rm t
}

# < redirect test -- requires grep and >
redirect_in_test(){
  echo -e "echo hello > t\ngrep he < t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '3q;d' $OUTFILE)

  if [ "$test_str" == "hello" ]; then
    let "correct"++
    echo -e "< redirect test -- requires > to work -- PASS\n"
  else
    echo -e "< redirect test -- requires > to work -- FAIL\n"
  fi
  
  rm $OUTFILE
  rm $ERRFILE
  rm t
}

# pipe test
pipe_test(){
  echo -e "echo hello world hello world | grep hello | wc -m\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '2q;d' $OUTFILE)
  if [ "$test_str" == "24" ]; then
    let "correct"++
    echo -e "pipe test -- PASS\n"
  else
    echo -e "pipe test -- FAIL\n"
  fi

  rm $OUTFILE
  rm $ERRFILE
}

# background & test
background_test(){
  echo -e "sleep 1&\nsleep 2\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '2q;d' $OUTFILE)
  test_str2=$(sed '1q;d' $ERRFILE)
  test_str3=$(sed '2q;d' $ERRFILE)
  if [ "$test_str" == "sshell$ sleep 2" ] &&
     [ "$test_str2" == "+ completed 'sleep 1&' [0]" ] &&
     [ "$test_str3" == "+ completed 'sleep 2' [0]" ]; then
     let "correct"++
     echo -e "Background test -- PASS\n"
  else
     echo -e "Background test -- FAIL\n"
  fi

  rm $OUTFILE
  rm $ERRFILE

}

# check for correct output on invalid command
invalid_cmd_test(){
  echo -e "d\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '1q;d' $ERRFILE)
  test_str2=$(sed '2q;d' $ERRFILE)
  if [ "$test_str" == "execvp: No such file or directory" ] &&
     [ "$test_str2" == "+ completed 'd' [1]" ]; then
     let "correct"++
     echo -e "Invalid cmd test -- PASS\n"
  else
     echo -e "Invalid cmd test -- FAIL\n"
  fi

  rm $OUTFILE
  rm $ERRFILE
}

# < redirect error test
invalid_in_test(){
  echo -e "echo h > t\ncat t | grep h < t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '2q;d' $ERRFILE)
  if [ "$test_str" == "Error: mislocated input redirection" ]; then
    let "correct"++
    echo -e "< redirect error test -- PASS\n"
  else
    echo -e "< redirect error test -- FAIL\n"
  fi

  rm t
  rm $OUTFILE
  rm $ERRFILE
}

# > redirect error test
invalid_out_test(){
  echo -e "echo h > t | cat t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '1q;d' $ERRFILE)
  if [ "$test_str" == "Error: mislocated output redirection" ]; then
    let "correct"++
    echo -e "> redirect error test -- PASS\n"
  else
    echo -e "> redirect error test -- FAIL\n"
  fi

  rm t
  rm $OUTFILE
  rm $ERRFILE
}

# & error test
invalid_background_test(){
  echo -e "echo hello & | grep hello\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE
  test_str=$(sed '1q;d' $ERRFILE)
  if [ "$test_str" == "Error: mislocated background sign" ]; then
    let "correct"++
    echo -e "& error test -- PASS\n"
  else
    echo -e "& error test -- FAIL\n"
  fi

  rm $OUTFILE
  rm $ERRFILE
}


# function that just runs every test
run_all_tests(){
  echo -e "\nBeginning tests. If script hangs, interrupt w/ Ctrl-C"
  echo -e "Note: May hang a little during background test\n"
  exit_test
  mkdir_test
  cd_and_pwd_test
  redirect_out_test
  redirect_in_test
  pipe_test
  invalid_cmd_test
  invalid_in_test
  invalid_out_test
  invalid_background_test
  background_test
}

main_func(){
  rm -rf $TDIR

  # setting up tests
  mkdir $TDIR
  cd $TDIR

  # run tests
  run_all_tests

  echo -e "$correct out of $total cases were passed\n"

  # clean up 
  cd ..
  rm -rf $TDIR
}

main_func


