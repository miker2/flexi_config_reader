# parser_lexer
A repo for testing various parser/lexer/grammar related tools


One such tool is `canopy`

On OSX this can be installed using:

```
brew install node
npm install -g canopy
```

The npm release of canopy has a small bug, which is fixed in the main repo.

Once canopy is installed, a python parser for a PEG can be produced using:

```
canopy maps.peg --lang python
```

The test script can then be invoked by:

```
python maps_test.py
```
