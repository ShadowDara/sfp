# sfp

the new parser for samfiles.

*NOT FINISHED YET, but will be better than the RUST Version*

[DOCS](https://shadowdara.github.io/docs)


## The Idea

the idea is to make a small *(binary size and install setup and config
files)* drop in tool for builds scripts and other config stuff which
is easy to use and extensible and fast. Extremly fast and easy cross
platform!

My Idea to solve this is one single file which can be splitted in
multiple sections.


## Sections


### Defaults

```
%%section TYPES

SAMFILE=SAMFILE
BATCH2=BATCH2
FLING=FLING
SETTINGS=KVP
TYPES=KVP

%%endsection
```

Every other section with is not listed there has no default type and will result
in a programm crash when called without defining their type. It will be possible
to change the type of the default types but please dont do this lol.

