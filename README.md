# ctl2-lgn

Logic gate network (torchlogix) classifying minbias vs ttbar vs hllhc-z events from 144 puppi candidates (pt, eta, phi).

## Data

For a quick start, `data/` ships small versions of `minbias.csv`, `ttbar.csv` and `hllhc-z.csv` (10k events each, ~50MB per file). Larger datasets can be generated with the Pythia8-based generator from the geawis repo (see `data/README.md` there):

```
git clone --filter=blob:none --no-checkout \
  git@github.com:SridharaDasu/geawis.git \
  geawis-tmp

cd geawis-tmp
git sparse-checkout init --cone
git sparse-checkout set data
git checkout main

mv data/ ..
cd ..
rm -rf geawis-tmp
```

Plots of each feature per process are in `data-discovery.ipynb`.

## Train

Trains a model from `models.py` (`LogicModelTiny/Small/Medium` or `BaselineMLP`) and logs config, metrics and weights to `runs/<model>_<timestamp>/`. Uses all events unless `--n-events` is given.

```
python train.py --model LogicModelSmall --epochs 20 --lr 0.01
```

## Plot training curves

Plots loss and accuracy (train, relaxed val, discrete val) to `<run>/training_curves.png`.

```
python plot_training.py runs/<run>
```

## Compile

Turns a trained logic model into a circuit, checks it against the PyTorch model on the validation set, and writes `circuit.c`, `circuit.v` and `thresholds.csv` to the run directory.

```
python compile_model.py runs/<run>
```
