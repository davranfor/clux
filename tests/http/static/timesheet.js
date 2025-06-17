const clocking = {
  time: null, timeout: null, elapsed: 0,
  text: document.getElementById('clocking-text'),
  update() {
    const now = Date.now();

    this.text.textContent = formatTime(now - this.time);
    this.timeout = setTimeout(() => this.update(), 1000 - (now % 1000));
  },
  start() {
    if (this.timeout) {
      clearTimeout(this.timeout);
      this.timeout = null;
    }
    this.time = Date.now() - (this.elapsed * 1000);
    this.text.textContent = '00:00:00';
    this.elapsed = 0;
    this.update();
  },
  stop() {
    if (this.timeout) {
      clearTimeout(this.timeout);
      this.timeout = null;
    }
    if (this.time) {
      this.text.textContent = '';
      this.time = Date.now();
    }
    this.elapsed = 0;
  }
}

async function logHours() {
  const response = await fetch('/api/timesheet', {
    method: 'POST',
    credentials: 'include'
  });

  if (response.status === 200) {
    const data = await response.json();
    user.workplace = data[0];
    user.clockIn = data[2] === 0 ? data[1] : 0;
  } else if (response.status === 204) {
    throw new Error('No se puede fichar en este momento');
  } else {
    const text = await response.text();
    throw new Error(text || `HTTP Error ${response.status}`);
  }
}

async function refreshClockingTable() {
  const response = await fetch('/api/timesheet/week', {
    method: 'GET',
    credentials: 'include'
  });

  if (response.status === 200) {
    const data = await response.json();
    refreshClockingTableUI(data);
  } else if (response.status !== 204) {
    const text = await response.text();
    throw new Error(text || `HTTP Error ${response.status}`);
  }
}

function refreshClockingTableUI(data) {
  const tbody = document.querySelector('#clocking-table tbody');

  tbody.replaceChildren();
  if (data.length === 0) {
    if (user.clockIn === 0) clockingTitle.textContent = 'No hay fichajes que mostrar';
    return;
  }
  data.reverse();
  data.forEach(record => {
    const dt1 = new Date(record[2].replace(' ', 'T'));
    const dt2 = new Date(record[3].replace(' ', 'T'));
    const tr1 = document.createElement('tr');
    const tr2 = document.createElement('tr');

    tr1.innerHTML = `
      <td class="editable" colspan="4" onclick="timesheetEdit(${record[0]});">
      <div><i class="ti ti-edit"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
      </td>
    `;
    tr2.innerHTML = `
      <td>${longDate(record[2])}</td>
      <td>${shortTime(record[2])}</td>
      <td>${shortTime(record[3])}</td>
      <td>${timeDiff(dt2, dt1)}</td>
    `;
    tbody.appendChild(tr1);
    tbody.appendChild(tr2);
    if (record[4] === null) {
      return;
    }

    const obj = JSON.parse(record[4]);
    if (Object.keys(obj).length > 1) {
      const dt3 = new Date(obj.clock_in.replace(' ', 'T'));
      const dt4 = new Date(obj.clock_out.replace(' ', 'T'));
      const tr3 = document.createElement('tr');
      const tr4 = document.createElement('tr');
      const tr5 = document.createElement('tr');

      tr3.innerHTML = `
        <td class="editable" colspan="4" onclick="timesheetRequestClear(${record[0]});">
        <div><i class="ti ti-refresh"></i><span>Modificado, ${obj.workplace_name}</span></div>
        </td>
      `;
      tr4.innerHTML = `
        <td>${longDate(obj.clock_in)}</td>
        <td>${shortTime(obj.clock_in)}</td>
        <td>${shortTime(obj.clock_out)}</td>
        <td>${timeDiff(dt4, dt3)}</td>
      `;
      tr5.innerHTML = `<td class="taLeft" colspan="4">${obj.reason}</td>`;
      tbody.appendChild(tr3);
      tbody.appendChild(tr4);
      tbody.appendChild(tr5);
    } else {
      const tr3 = document.createElement('tr');
      const tr4 = document.createElement('tr');

      tr3.innerHTML = `
        <td class="editable" colspan="4" onclick="timesheetRequestClear(${record[0]});">
        <div><i class="ti ti-trash"></i><span>Pendiente de eliminar</span></div>
        </td>
      `;
      tr4.innerHTML = `<td class="taLeft" colspan="4">${obj.reason}</td>`;
      tbody.appendChild(tr3);
      tbody.appendChild(tr4);
    }
  });
}

const timesheet = {
  hash: 0,
  id: clockingForm.querySelector('input[name="id"]'),
  workplace: clockingForm.querySelector('select[name="workplace"]'),
  clockIn: {
    date: clockingForm.querySelector('input[name="clock_in_date"]'),
    time: clockingForm.querySelector('input[name="clock_in_time"]')
  },
  clockOut: {
    date: clockingForm.querySelector('input[name="clock_out_date"]'),
    time: clockingForm.querySelector('input[name="clock_out_time"]')
  },
  reason: clockingForm.querySelector('input[name="reason"]'),
}

async function timesheetEdit(id) {
  try {
    const response = await fetch(`/api/timesheet/${id}/edit`, {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();
      timesheetEditUI(data);
    } else if (response.status === 204) {
      showMessage('El registro ya no existe');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP Error ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede editar el registro');
  }
}

function timesheetEditUI(data) {
  clockingTable.style.display = "none";
  clockingForm.style.display = "flex";

  timesheet.id.value = data.id;

  timesheet.workplace.replaceChildren();
  data.workplaces.forEach(workplace => {
    const option = document.createElement('option');

    option.value = workplace[0];
    option.textContent = workplace[1];
    timesheet.workplace.appendChild(option);
  });
  timesheet.workplace.value = data.workplace_id;

  let date, time;
  [date, time] = pairDateTime(data.clock_in);
  timesheet.clockIn.date.value = date;
  timesheet.clockIn.time.value = time;
  [date, time] = pairDateTime(data.clock_out);
  timesheet.clockOut.date.value = date;
  timesheet.clockOut.time.value = time;

  timesheet.reason.value = "";

  timesheet.hash = formHash(clockingForm);
}

document.querySelectorAll('#clocking-form input').forEach(element => {
  element.addEventListener('blur', function() {
    this.value = this.value.trim();
  });
  element.addEventListener('keydown', function(e) {
    if (e.key === 'Enter') {
      this.value = this.value.trim();
    }
  });
});

clockingForm.addEventListener('submit', (e) => {
  e.preventDefault();

  const hash = formHash(clockingForm);

  if (timesheet.hash === hash) {
    showMessage("No se ha realizado ningún cambio en el registro");
    return;
  }
  timesheet.hash = hash;

  const workplace_id = Number(timesheet.workplace.value);
  const clock_in = `${timesheet.clockIn.date.value} ${timesheet.clockIn.time.value}:00`;
  const clock_out = `${timesheet.clockOut.date.value} ${timesheet.clockOut.time.value}:00`;

  const dt1 = new Date(clock_in.replace(' ', 'T'));
  const dt2 = new Date(clock_out.replace(' ', 'T'));

  if (dt1 >= dt2) {
    showMessage("La hora de entrada no puede ser igual o superior a la de salida").then(() => {
      focusedField.focus();
    });
    return;
  }
  if (dt2 - dt1 > 9 * 60 * 60 * 1000) {
    showMessage("No pueden transcurrir más de 9 horas entre la hora de entrada y la de salida").then(() => {
      focusedField.focus();
    });
    return;
  }
  if (user.role !== role.ADMIN) {
    const workplace_name = timesheet.workplace.options[timesheet.workplace.selectedIndex].text;
    const reason = timesheet.reason.value;

    timesheetRequestUpdate(JSON.stringify({ workplace_id, workplace_name, clock_in, clock_out, reason }));
  } else {
    timesheetUpdate(JSON.stringify({ workplace_id, clock_in, clock_out }));
  }
});

document.getElementById("clocking-delete").addEventListener("click", () => {
  if (user.role !== role.ADMIN) {
    const reason = timesheet.reason.value;

    if (reason === "") {
      showMessage("Especifica el motivo para eliminar el registro").then(() => {
        timesheet.reason.focus();
      });
      return;
    }
    timesheetRequestDelete(JSON.stringify({ reason }));
  } else {
    timesheetDelete();
  }
});

document.getElementById("clocking-cancel").addEventListener("click", () => {
  clockingForm.style.display = "none";
  clockingTable.style.display = "table";
});

async function timesheetRequestUpdate(data) {
  try {
    const response = await fetch(`/api/timesheet/${timesheet.id.value}/update`, {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      credentials: 'include',
      body: data
    });

    if (response.status === 200) {
      clockingForm.style.display = "none";
      clockingTable.style.display = "table";
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No hay ningún cambio que solicitar');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede actualizar el registro');
  }
}

async function timesheetUpdate(data) {
  try {
    const response = await fetch(`/api/timesheet/${timesheet.id.value}/update`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      credentials: 'include',
      body: data
    });

    if (response.status === 200) {
      clockingForm.style.display = "none";
      clockingTable.style.display = "table";
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No hay ningún cambio que guardar');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede actualizar el registro');
  }
}

async function timesheetRequestDelete(reason) {
  try {
    const response = await fetch(`/api/timesheet/${timesheet.id.value}/delete`, {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      credentials: 'include',
      body: reason
    });

    if (response.status === 200) {
      clockingForm.style.display = "none";
      clockingTable.style.display = "table";
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No hay nada que eliminar');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede actualizar el registro');
  }
}

async function timesheetDelete() {
  if (!await confirmMessage("Se eliminará el registro seleccionado")) {
    return;
  }
  try {
    const response = await fetch(`/api/timesheet/${timesheet.id.value}`, {
      method: 'DELETE',
      credentials: 'include'
    });

    if (response.status === 200) {
      clockingForm.style.display = "none";
      clockingTable.style.display = "table";
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No hay nada que eliminar');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede actualizar el registro');
  }
}

async function timesheetRequestClear(id) {
/*
  const nombre = await promptMessage("Por favor ingrese su nombre:", "John Doe");

  if (nombre === null) {
    return;
  }
*/
  if (!await confirmMessage("Se eliminará la solicitud pendiente")) {
    return;
  }
  try {
    const response = await fetch(`/api/timesheet/${id}/clear`, {
      method: 'PATCH',
      credentials: 'include'
    });

    if (response.status === 200) {
      clockingForm.style.display = "none";
      clockingTable.style.display = "table";
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No hay nada que actualizar');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede actualizar el registro');
  }
}

function sumDays(date, days) {
  const result = new Date(date);

  result.setDate(result.getDate() + days);
  return result;
}

function refreshScheduleTable() {
  const tbody = document.querySelector('#schedule-table tbody');

  tbody.replaceChildren();

  const today = new Date();

  const workplaces = [
    [ 1, "SEDE CENTRAL" ],
    [ 2, "PALMANOVA" ],
    [ 3, "MAGALLUF" ]
  ];
  const options = workplaces.map(t =>`<option value="${t[0]}">${t[1]}</option>`).join('');

  function createSelect() {
    const select = document.createElement('select');

    select.innerHTML = options;
    return select;
  }

  const tr = document.createElement('tr');

  tr.innerHTML = '<th>Fecha</th><th colspan="2">Primer turno</th><th colspan="2">Segundo turno</th>';
  tbody.appendChild(tr);
  for (let i = 0; i < 14; i++) {
    const tr1 = document.createElement('tr');
    const tr2 = document.createElement('tr');

    date = sumDays(today, i);
    tr1.innerHTML = `
      <td rowspan="2" class="date">${formatDate(date)}<br>${dayOfWeek(date)}</td>
      <td colspan="2" class="options"></td>
      <td colspan="2" class="options"></td>
    `;

    const [select1, select2] = tr1.querySelectorAll('.options');

    select1.appendChild(createSelect());
    select2.appendChild(createSelect());

    tr2.innerHTML = `
      <td class="entry"><input type="time" maxLength="5" value="07:00"></td>
      <td class="entry"><input type="time" maxLength="5" value="15:00"></td>
      <td class="entry"><input type="time" maxLength="5" value="00:00"></td>
      <td class="entry"><input type="time" maxLength="5" value="00:00"></td>
    `;
    tbody.appendChild(tr1);
    tbody.appendChild(tr2);
  }
}

