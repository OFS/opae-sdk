import json
from flask import Flask
from opae import fpga

app = Flask('opae.io')

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
    return info
    # return dict(((a, getattr(p, a))
    #             for a, types in attrs.items() if p.type in types))


@app.route('/api/v0/status')
def status():
    return 'ok'


@app.route('/api/v0/fpga')
def enum():
    tokens = fpga.enumerate()
    return json.dumps([token_info(t) for t in tokens])


def run(args):
    app.run(host='0.0.0.0', port=8080)


def __main__():
    run(None)


if __name__ == '__main__':
    t = fpga.enumerate()
    run(None)


