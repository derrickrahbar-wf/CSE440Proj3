    #!/bin/bash
        for i in `seq 2 $1`; do
            ./test_proj.sh testcases/testmd-$i > testcases/expected/testmd-$i-expected
        done