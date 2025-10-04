import requests
import zipfile
import os

if not os.path.exists('temp'):
    os.mkdir('temp')

json_stream = requests.get('https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip')
with open('temp/json-v3.12.0.zip', 'wb') as f:
    f.write(json_stream.content)

with zipfile.ZipFile('temp/json-v3.12.0.zip', 'r') as zip_ref:
    zip_ref.extractall()

os.rename('json-3.12.0', 'json')
