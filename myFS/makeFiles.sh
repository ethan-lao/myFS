for i in {1..8000}
do
    touch f$i.txt
    echo "test" >> f$i.txt
done