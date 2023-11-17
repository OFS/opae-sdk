This is standalone signing tool

You need to have Python 3.5/3.6 (tested) to run the script

You can run pacsign-tests.sh to fully execute all the available operation

    $ python3 -m virtualenv pacsign-venv
    $ source ./pacsign-venv/bin/activate
    $ pip3 install ./opae-sdk/python/pacsign
    $ ./opae-sdk/python/pacsign/pacsign-tests.sh
    $ deactivate

