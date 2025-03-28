- [x] Add option to stop on first non-AC testdata of each task (TODO FEATURE(group))
- [x] Download compressed testdata
- [x] User-specified compilation flags (special judge / user program)
- [ ] SIGINT waits for current judge queue to complete
- [ ] Kattis / Polygon / CMS(?)-style special judge
- [ ] Pin judge (platform-independent judge result)
- [ ] I/O Interactive (TODO FEATURE(io-interactive))
    - I/O interactive should also support multistage (similar to that of CMS Communication)
    - Modify cjail to allow specify cgroup name for CPU time / memory accounting (set a hard limit for sum of all tasks to avoid attacks)
        - Add cgroup_settings & cgroup_base_name to struct cjail_ctx; implement runtime detection of cgroups (for cgroup_settings = NULL)
    - Treat all processes as one task
- [ ] Output-only
    - Needs a lot of modifications on web server and API
- [x] Optimize task result sending (not send every task result every time)
- [x] Refactor: add testdata path into `class Submission` and move related paths out of libtioj
- [ ] Periodically clear testdata for recently unused problems
- [ ] Add more languages
- [ ] Add non-root judge (use libfakechroot for isolation; no RSS limiting & VSS reporting support)
- [ ] Add command-line utility for judging
- [ ] Cache special judge compile results
- [ ] Reference program for time calibration
