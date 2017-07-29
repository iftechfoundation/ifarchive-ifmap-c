# ifmap -- the index generator tool for the IF Archive

- Copyright 2017 by the Interactive Fiction Technology Foundation
- Distributed under the MIT license
- Created by Andrew Plotkin <erkyrath@eblong.com>

This program has one core task: to look through all the files in the IF Archive, combine that with the contents of the Master-Index file, and generate all the index.html files in the indexes subdirectory.

(The Master-Index file is created by sewing together all the Index files in all the directories of the Archive. A different script does that job.)

This version has been retired in favor of [ifarchive-ifmap-py][].

[ifarchive-ifmap-py]: https://github.com/iftechfoundation/ifarchive-ifmap-py

## Arguments

In normal Archive operation, this is invoked from the build-indexes script.

- -index FILE: pathname of Master-Index. (Normally /var/ifarchive/htdocs/if-archive/Master-Index.)
- -src DIR: Pathname of the directory full of HTML templates which control the appearance of the index files. (Normally /var/ifarchive/lib/ifmap.)
- -dest DIR: Pathname of the indexes directory, where the index files are written. (Normally /var/ifarchive/htdocs/indexes.)
- -tree DIR: Pathname of the root directory which the Archive serves. (Normally /var/ifarchive/htdocs.)
- -v: If set, print verbose output.
- -xml: If set, also create a Master-Index.xml file (in the indexes directory) which includes all the known metadata. (Normally set.)
- -exclude: If set, files without index entries are excluded from index listings. (Normally *not* set.)

## History

I wrote the first version of this program in 1999-ish. It was built around the original Index files, which were hand-written by Volker Blasius (the original Archive curator) for human consumption. Their format was not particularly convenient for parsing, but I parsed them anyway.

I wrote the program in C because it was portable and I didn't know Python or Perl yet. C is a terrible language for this sort of thing, of course -- I started by implementing my own hash tables. And escaping strings for HTML? Yuck.

There are plenty of quirks and limitations which persist because C is too hard to update. Now that [ifarchive-ifmap-py][] exists, nobody has to.


