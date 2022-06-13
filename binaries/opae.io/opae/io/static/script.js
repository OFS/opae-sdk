const enum_output = document.querySelector('.enum_output');
const refresh = document.querySelector('.refresh');
const enumerate_url = '/api/v0/fpga';
const upload_timeout = 30000;

refresh.onclick = ()=> {
  enum_output.innerHTML = '';
  fetchEnumerationData();
}

function fetchEnumerationData() {
  fetch(enumerate_url)
  .then(rep => rep.json())
  .then(data =>{
    outputEnumerationData(enum_output, data);
  })
}

function outputEnumerationData(out, value) {
  console.log(value);
  let html = '<table border=1 >';

  html += '<tr>';
    html += '<th>Reconfigure</th>';
    html += '<th>Type</th>';
    html += '<th>VID:DID</th>';
    html += '<th>SVID:SDID</th>';
    html += '<th>PCIe Address</th>';
    html += '<th>GUID</th>';
    html += '<th>Interface</th>';
    //html += '<th>Errors</th>';
    html += '<th>Object ID</th>';
    html += '<th></th>';
  html += '</tr>';

  value.forEach((value_el, value_i) => {

    fpga = value[value_i];
    let is_fim = false;

    if (fpga.type.localeCompare('fpga_objtype.DEVICE') == 0)
      is_fim = true;

    let object_id = Number(fpga.object_id).toString(16).padStart(16, '0');

    html += '<tr>';

      html += '<td>';
      if (is_fim) {
        html += '<div id="drop_zone"\n';
        html += '  ondrop="dropGBSHandler(event, ' + "'" + object_id + "'" + ');"\n';
        html += '  ondragover="dragOverGBSHandler(event);">\n';
        html += '<p>Drop GBS to PR</p>\n';
        html += '</div>';
      }
      html += '</td>';

      html += '<td>';
      if (fpga.type.localeCompare('fpga_objtype.DEVICE') == 0)
        html += 'FIM';
      else
        html += 'AFU';
      html += '</td>';

      html += '<td>';
        html += Number(fpga.vendor_id).toString(16).padStart(4, '0') + ':' +
                Number(fpga.device_id).toString(16).padStart(4, '0');
      html += '</td>';

      html += '<td>';
        html += Number(fpga.subsystem_vendor_id).toString(16).padStart(4, '0') + ':' +
                Number(fpga.subsystem_device_id).toString(16).padStart(4, '0');
      html += '</td>';

      let pci_addr = Number(fpga.segment).toString(16).padStart(4, '0') + ':' +
                     Number(fpga.bus).toString(16).padStart(2, '0') + ':' +
                     Number(fpga.device).toString(16).padStart(2, '0') + '.' +
                     Number(fpga.function).toString(10);

      html += `<td>` + pci_addr + `</td>`;

      html += `<td>${fpga.guid}</td>`;

      html += '<td>';
      if (fpga.interface.localeCompare('fpga_interface.IFC_DFL') == 0)
        html += 'DFL';
      else if (fpga.interface.localeCompare('fpga_interface.IFC_VFIO') == 0)
        html += 'VFIO';
      else if (fpga.interface.localeCompare('fpga_interface.IFC_SIM') == 0)
        html += 'ASE';
      html += '</td>';

      //let num_errors = Number(fpga.num_errors).toString(10);
      //html += `<td>${num_errors}</td>`;

      html += `<td>${object_id}</td>`;

      html += '<td>';
      if (is_fim) {

        let bbs_id = Number(fpga.bbs_id).toString(16).padStart(16, '0');
        html += `bbs_id=${bbs_id};`;

        let bbs_version = Number(fpga.bbs_version[0]).toString(10) + '.' +
                          Number(fpga.bbs_version[1]).toString(10) + '.' +
                          Number(fpga.bbs_version[2]).toString(10);
        html += `bbs_version=${bbs_version};`;

        let num_slots = Number(fpga.num_slots).toString(10);
        html += `slots=${num_slots}`;

      } else {

        html += 'state=';
        if (fpga.accelerator_state.localeCompare("fpga_accelerator_state.ACCELERATOR_ASSIGNED") == 0)
          html += 'in use';
        else
          html += 'free';

        if ('num_interrupts' in fpga) {
          let num_irqs = Number(fpga.num_interrupts).toString(10);
          html += `;IRQs=${num_irqs}`;
        }

        if ('num_mmio' in fpga) {
          let num_mmio = Number(fpga.num_mmio).toString(10);
          html += `;MMIOs=${num_mmio}`;
        }

      }
      html += '</td>';

    html += '</tr>';

  });

  html += '</table>';
  out.innerHTML = html;
}

async function dropGBSHandler(ev, object_id) {
  console.log('File(s) dropped');

  // Don't open the files in the browser..
  ev.preventDefault();

  var file = null;

  if (ev.dataTransfer.items) {

    if (ev.dataTransfer.items.length > 0) {
      file = ev.dataTransfer.items[0].getAsFile();
    }

    //for (var i = 0; i < ev.dataTransfer.items.length; i++) {
    //  if (ev.dataTransfer.items[i].kind === 'file') {
    //    var file = ev.dataTransfer.items[i].getAsFile();
    //    console.log('... file[' + i + '].name = ' + file.name);
    //  }
    //}

  } else {

    if (ev.dataTransfer.files.length > 0) {
      file = ev.dataTransfer.files[0];
    }

    //for (var i = 0; i < ev.dataTransfer.files.length; i++) {
    //  console.log('... file[' + i + '].name = ' + ev.dataTransfer.files[i].name);
    //}

  }

  if (file != null) {
    console.log('... file[0].name = ' + file.name + ' for ' + object_id);

    let formData = new FormData();
    formData.append('gbs', file);

    const ctrl = new AbortController();
    setTimeout(() => ctrl.abort(), upload_timeout);

    try {

      let r = await fetch(`/api/v0/fpga/${object_id}/pr`,
                          {method: "POST", body: formData, signal: ctrl.signal});

      console.log('response code: ', r.status);

    } catch(e) {
      console.log('Whoops! ' + e);
    }

  }
}

function dragOverGBSHandler(ev) {
  // Don't open the file in the browser..
  ev.preventDefault();
}
