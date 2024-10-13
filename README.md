It is a simple word generator to generate fake english words whitch look like real ones.

## Output examples
```
nebullient sealcondorm
glowinkling tigersharkey
fantous donkey
thorcellate buzzardvark
diappering crabatah
quious shermoseal
belegassive lizardwolfly
quious permwut
twinkling hamelle
nebulous pigersharkey
```

## How to use an example

After you compile the code, you can run`./markow_word_gen`**`x y`** from a ```build``` dirrectory. 
Where are:
* x - maximum model [order](https://en.wikipedia.org/wiki/Katz%27s_back-off_model)
* y - ```gain``` is a chance for an algorithm to picks an accurate Markov's chain letter instead of random one. As smaller gain as more chaos you can see in generated words.
