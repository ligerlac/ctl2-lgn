# Data generation

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
