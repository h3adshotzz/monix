# Monix Contribution Guidelines

The Monix kernel is hosted on Github, with a mirror being available soon on cgit.h3adsh0tzz.com.

Contribution are welcomed and encouraged. The process for making changes is to fork the repository and submit a pull request. The pull request will be tested and reviewed by the maintainer, and if it is accepted the changes will be merged onto the main branch.

### Commit messages

A commit title should begin with the component being modified, followed by a short description of the change. A more detail explanation can follow in the body of the commit, and detail depends on how self-explanatory the changes are. Finally, commits must include a `Signed-off-by` signature. For example:
```
kern/sched: schedular performance improvements

<detailed description of change>

Signed-off-by: Harry Moulton <me@h3adsh0tzz.com>
```

### Code style

Currently there is no formal code style. Ideally keep the format relatively similar to the rest of the file being modified.