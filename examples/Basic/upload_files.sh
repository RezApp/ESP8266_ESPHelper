#!/bin/sh

for file in ./data/*
do
  curl -F file=@${file} $1/upload
done