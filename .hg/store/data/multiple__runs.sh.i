         J   I      $
�������� Ȏ���zWad�V�DR�Xg            u#!/bin/bash

for i in {0..7}
do
    ./run_exp.sh $(($i*1000 + 1)) &
done
