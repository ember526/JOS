# JOS

                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
                     |                              |
                     |            ......            |
 UTOP,UENVS ------>  +------------------------------+ 0xeec00000
 UXSTACKTOP -/       |     User Exception Stack     | RW/RW  PGSIZE
                     +------------------------------+ 0xeebff000
                     |       Empty Memory (*)       | --/--  PGSIZE
    USTACKTOP  --->  +------------------------------+ 0xeebfe000
                     |     User Stack TID = 0       | RW/RW  PGSIZE
                     +------------------------------+ 0xeebfd000
                     |        Invalid Memory        | RW/RW  PGSIZE
                     +------------------------------+ 0xeebfc000
                     |     User Stack TID = 1       | RW/RW  PGSIZE
                     +------------------------------+ 0xeebfb000
                     |        Invalid Memory        | RW/RW  PGSIZE
                     +------------------------------+ 0xeebfa000
                     |                              |
	                   |            ......            |
                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
                     |     Program Data & Heap      |
    UTEXT -------->  +------------------------------+ 0x00800000
    
    
