## PGPRO Practice repos

##### This repository contains the PostgreSQL extension **pg_array_utils** developed during the practice and other files which have been / are being / will be used to solve the PGPRO Practice tasks.
Current repository contents:
- [pg_array_utils](pg_array_utils/) - the extension folder:
    - [sql/](pg_array_utils/sql/)      - regression tests;
    - [expected/](pg_array_utils/expected/) - expected results for regression tests;
    - [src/](pg_array_utils/src/)      - source C code;
    - [pg_array_utils--1.0.sql](pg_array_utils/pg_array_utils--1.0.sql)
    - [pg_array_utils.control](pg_array_utils/pg_array_utils.control)
    - [Makefile](pg_array_utils/Makefile)
    Folder also contains [design_doc.md](pg_array_utils/design_doc.md) - design document which describes extension functions;
- [introduce_tasks/](introduce_tasks/) - folder with files, needed for tasks, completed before the development of the extension:
    - [task1/](introduce_tasks/task1/), [task2/](introduce_tasks/task2/), [task3/](introduce_tasks/task3/), [task4/](introduce_tasks/task4/), [task5/](introduce_tasks/task5/);
    - [python_benchmark_scripts/](introduce_tasks/python_benchmark_scripts/) - folder with python scripts for task5.
