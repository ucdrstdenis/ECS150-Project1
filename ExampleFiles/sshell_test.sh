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

# binaries
RM="rm -f"	# don't fail if file doesn't exist

# Test correct exiting
exit_test(){
  #echo -e "exit\n" | ../sshell 1> ../out_test 2> ../err_test
  echo -e "exit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  corr_str="Bye..."

  echo -n "exit_test -- "

  if [ "$test_str" == "$corr_str" ]; then
    let "correct++"
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  # clean up
  $RM $OUTFILE
  $RM $ERRFILE
}

# Tests for correct mkdir output
mkdir_test(){
  test_dir="test_d"

  echo -e "mkdir -p $test_dir\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  corr_str="+ completed 'mkdir -p $test_dir' [0]"

  echo -n "mkdir test -- output only, not functionality -- "

  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  # clean up
  $RM $OUTFILE
  $RM $ERRFILE
  $RM -r "$test_dir"
}

# cd and pwd test
cd_and_pwd_test(){
  test_dir="test_d"

  echo -e "mkdir -p $test_dir\ncd $test_dir\npwd\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  corr_str="+ completed 'mkdir -p $test_dir' [0]"
  test_str2=$(sed '4q;d' $OUTFILE)
  corr_str2="$path/$TDIR/$test_dir"

  echo -n "cd and pwd -- "

  if [ "$test_str" == "$corr_str" ] &&
     [ "$test_str2" == "$corr_str2" ]; then
     let "correct"++
     echo "PASS"
  else
     echo "FAIL\n"
    echo "Got '$test_str' but expected '$corr_str'"
    echo "Got '$test_str2' but expected '$corr_str2'"
  fi
  echo

  cd $path/$TDIR
  $RM -r "$test_dir"
  $RM $OUTFILE
  $RM $ERRFILE

}

pwd_syscall_test() {
  # for checking if pwd was a syscall, outfile should be empty if syscall
  strace -f sh -c 'echo -e "pwd\nexit\n" | ../sshell' |& grep "usr/bin/pwd" > $OUTFILE

  test_str=$(sed '1q;d' $OUTFILE)
  corr_str=""

  echo -n "pwd is syscall check -- "

  if [ "$test_str" == "$corr_str" ]; then
     let "correct"++
     echo "PASS"
  else
     echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE

}

# > redirect test -- requires cat and echo
redirect_out_test(){
  echo -e "echo hello > t\ncat t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '3q;d' $OUTFILE)
  corr_str="hello"

  echo -n "> redirect test -- "

  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE
  $RM t
}

# < redirect test -- requires grep and >
redirect_in_test(){
  echo -e "echo hello > t\ngrep he < t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '3q;d' $OUTFILE)
  corr_str="hello"

  echo -n "< redirect test -- requires > to work -- "

  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE
  $RM t
}

# pipe test
pipe_test(){
  echo -e "echo hello world hello world | grep hello | wc -m\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '2q;d' $OUTFILE)
  corr_str="24"

  echo -n "pipe test -- "
  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE
}

# background & test
background_test(){
  echo -e "sleep 1&\nsleep 2\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '2q;d' $OUTFILE)
  corr_str="sshell$ sleep 2"
  test_str2=$(sed '1q;d' $ERRFILE)
  corr_str2="+ completed 'sleep 1&' [0]"
  test_str3=$(sed '2q;d' $ERRFILE)
  corr_str3="+ completed 'sleep 2' [0]"

  echo -n "Background test -- "
  if [ "$test_str" == "$corr_str" ] &&
     [ "$test_str2" == "$corr_str2" ] &&
     [ "$test_str3" == "$corr_str3" ]; then
     let "correct"++
     echo "PASS"
  else
     echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
    echo "Got '$test_str2' but expected '$corr_str2'"
    echo "Got '$test_str3' but expected '$corr_str3'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE

}

# check for correct output on invalid command
invalid_cmd_test(){
  echo -e "d\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  corr_str="execvp: No such file or directory"
  test_str2=$(sed '2q;d' $ERRFILE)
  corr_str2="+ completed 'd' [1]"

  echo -n "Invalid cmd test -- "
  if [ "$test_str" == "$corr_str" ] &&
     [ "$test_str2" == "$corr_str2" ]; then
     let "correct"++
     echo "PASS"
  else
     echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
    echo "Got '$test_str2' but expected '$corr_str2'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE
}

# < redirect error test
invalid_in_test(){
  echo -e "echo h > t\ncat t | grep h < t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '2q;d' $ERRFILE)
  corr_str="Error: mislocated input redirection"

  echo -n "< redirect error test -- "

  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM t
  $RM $OUTFILE
  $RM $ERRFILE
}

# > redirect error test
invalid_out_test(){
  echo -e "echo h > t | cat t\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  corr_str="Error: mislocated output redirection"

  echo -n "> redirect error test -- "

  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM t
  $RM $OUTFILE
  $RM $ERRFILE
}

# & error test
invalid_background_test(){
  echo -e "echo hello & | grep hello\nexit\n" | ../sshell 1> $OUTFILE 2> $ERRFILE

  test_str=$(sed '1q;d' $ERRFILE)
  corr_str="Error: mislocated background sign"

  echo -n "& error test -- "
  if [ "$test_str" == "$corr_str" ]; then
    let "correct"++
    echo "PASS"
  else
    echo "FAIL"
    echo "Got '$test_str' but expected '$corr_str'"
  fi
  echo

  $RM $OUTFILE
  $RM $ERRFILE
}


# function that just runs every test
run_all_tests(){
  echo -e "\nBeginning tests. If script hangs, interrupt w/ Ctrl-C"
  echo -e "Note: May hang a little during background test\n"
  exit_test
  mkdir_test
  cd_and_pwd_test
  pwd_syscall_test
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
  $RM -r $TDIR

  # setting up tests
  mkdir -p $TDIR
  cd $TDIR

  # run tests
  run_all_tests

  echo -e "$correct out of $total cases were passed\n"

  # clean up 
  cd ..
  $RM -r $TDIR
}

main_func


