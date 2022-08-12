#! /bin/bash
echo Compiling
g++ main.cpp -o bin

echo "Welcome to VISA-BR_INCOMING-FILE_PARSER"
echo "Reading files from reports folder"
echo "Running"
./bin
echo "Finished execution output.csv were created."
