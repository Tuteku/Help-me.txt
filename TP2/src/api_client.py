import requests
import sys

URL = "https://api.worldbank.org/v2/en/country/all/indicator/SI.POV.GINI?format=json&date=2011:2020&per_page=32500&page=1&country=%22Argentina%22"

res = requests.get(URL)
if res:
    print('Response OK', file=sys.stderr)
else:
    print('Response Failed', file=sys.stderr)
    sys.exit(1)

data = res.json()
# data[0] = metadata, data[1] = lista de registros
records = data[1]

for idx, entry in enumerate(records):
    value = entry.get("value")
    if value is None or entry["country"]["value"] != "Argentina":
        continue
    country = entry["country"]["value"]
    date = entry["date"]
    print(f"{country};{idx};{float(value):.4f};{date}")
