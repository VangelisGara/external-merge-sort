## External Merge Sort
Implementation of the external merge algorithm, based on Navathe's and Elmarsi's algorithm.

The algorithm is used for big text files that doesn't fit in memory. It starts by sorting small subfiles, runs, and then mergers them into a bigger sorted files.

The first phase of the algorithms is the Sort Phase, while each run is loaded into the memory, buffer, and sorted with Quicksort into temp sub files. The second phase is the merge phase, where runs will be merged into a bigger sorted runs, creating finally the sorted file.

## To compile & Execute
    make compile
    ./build/sr_main1 , to created an unsorted file
    ./build/sr_main2 , to sort files based on specific field
    ./build/sr_main3 , to print the sorted files based on each field
