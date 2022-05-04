make test
cd mountpoint
touch foo
cp ../program .
cp ../makeFiles.sh .
chmod +x program

time bash makeFiles.sh

echo "
"
time ./program 1 26

echo "
"
time ./program 2 26

echo "
"
time ./program 3 24

echo "
"
time ./program 4 11
