import json
import os
from pprint import pprint
import subprocess
import sys
import tempfile
import time
from flask import Flask, request, abort, jsonify
from opae import fpga

DEFAULT_SRV_PORT = 8080

app = Flask('opae.io', static_folder='static')

attrs = {
        'accelerator_state': (fpga.ACCELERATOR, ),
        'bbs_id': (fpga.DEVICE, ),
        'bbs_version': (fpga.DEVICE, ),
        'bus': (fpga.DEVICE, fpga.ACCELERATOR),
        'capabilities': (fpga.DEVICE, ),
        'device': (fpga.DEVICE, fpga.ACCELERATOR),
        'device_id': (fpga.DEVICE, fpga.ACCELERATOR),
        'function': (fpga.DEVICE, fpga.ACCELERATOR),
        'guid': (fpga.DEVICE, fpga.ACCELERATOR),
        'interface': (fpga.DEVICE, fpga.ACCELERATOR),
        'local_memory_size': (),
        'model': (fpga.DEVICE, fpga.ACCELERATOR),
        'num_errors': (fpga.DEVICE, fpga.ACCELERATOR),
        'num_interrupts': (fpga.ACCELERATOR, ),
        'num_mmio': (fpga.ACCELERATOR, ),
        'num_slots': (fpga.DEVICE, ),
        'object_id': (fpga.DEVICE, fpga.ACCELERATOR),
        'parent': (fpga.ACCELERATOR, ),
        'segment': (fpga.DEVICE, fpga.ACCELERATOR),
        'socket_id': (fpga.DEVICE, fpga.ACCELERATOR),
        'subsystem_device_id': (fpga.DEVICE, fpga.ACCELERATOR),
        'subsystem_vendor_id': (fpga.DEVICE, fpga.ACCELERATOR),
        'type': (fpga.DEVICE, fpga.ACCELERATOR),
        'vendor_id': (fpga.DEVICE, fpga.ACCELERATOR),
}


# For grafana SimpleJson data source

def now_in_milliseconds():
    return int(time.time()) * 1000


def get_sensors_json():
    tmpfile = tempfile.NamedTemporaryFile(mode='w+',
        encoding=sys.getdefaultencoding(), newline='\n')
    subprocess.run(['sensors', '-j'], stdout=tmpfile, stderr=subprocess.DEVNULL)
    tmpfile.seek(0, os.SEEK_SET)
    data = json.load(tmpfile)
    tmpfile.close()
    return data


@app.route('/', methods=['GET', 'POST'])
def test():
    return 'OK'

datapoints = {}

@app.route('/search', methods=['GET', 'POST'])
def search():
    j = request.get_json()

    result = []
    for x in j['targets']:
        target = x['target']
        if target not in datapoints:
            datapoints[target] = []
        result.append(target)

    return jsonify(result)


@app.route('/query', methods=['GET', 'POST'])
def query():
    j = request.get_json()
    s = get_sensors_json()
    n = s['n3000bmc_hwmon-isa-0000']

    result = []
    for x in j['targets']:
        target = x['target']
        if target not in datapoints:
            datapoints[target] = []

        value = 0.0
        for k in n[target].keys():
            if '_input' in k:
                value = n[target][k]
                break

        datapoints[target].append([value, now_in_milliseconds()])
        result.append({'target': target, 'datapoints': datapoints[target] })

    return jsonify(result)

# End grafana SimpleJson data source


def token_info(t: fpga.token):
    p = fpga.properties(t)
    info = {}
    for a, types in attrs.items():
        if p.type in types:
            try:
                if a == 'parent':
                    info[a] = fpga.properties(p.parent).guid
                elif a in ('interface', 'type', 'accelerator_state'):
                    info[a] = str(getattr(p, a))
                else:
                    info[a] = getattr(p, a)
            except RuntimeError as err:
                print(a, err)
    #info['fpga_url'] = request.base_url.replace(remove_from_url, '')
    return info


@app.route('/api/v0/status')
def status():
    return 'ok'


@app.route('/api/v0/fpga', methods=['GET'])
def enum():
    tokens = fpga.enumerate()
    return json.dumps([token_info(t) for t in tokens])


@app.route('/api/v0/fpga/<object_id>/pr', methods=['POST'])
def partial_reconfig(object_id):
    file = request.files['gbs']
    if file:
        print(f'received {file.filename} to PR object_id={object_id}')
        t = fpga.enumerate(object_id=int(object_id, 16))
        if not t:
            abort(404)
        print(f'Starting PR..')
        try:
            with fpga.open(t[0], 0) as handle:
                handle.reconfigure(0, file)
            print(f'PR Complete')
        except RuntimeError as err:
            print(err)
    return 'ok'


def run(args):
    app.run(host='0.0.0.0', port=args.port)


def __main__():
    run({'port': DEFAULT_SRV_PORT})


if __name__ == '__main__':
    run({'port': DEFAULT_SRV_PORT})
