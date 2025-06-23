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

  const trNew = document.createElement('tr');

  trNew.innerHTML = `
    <td class="editable" colspan="4" onclick="timesheetNew();">
    <div><i class="ti ti-clock"></i><span>Nuevo fichaje</span></div>
    </td>
  `;
  tbody.appendChild(trNew);

  // Sort by clock_in DESC
  data.sort((a, b) => {
    if (a[3] < b[3]) return 1;
    if (a[3] > b[3]) return -1;
    return 0;
  });
  data.forEach(record => {
    const trWorkplace = document.createElement('tr');
    const dt1 = new Date(record[3].replace(' ', 'T'));
    const dt2 = new Date(record[4].replace(' ', 'T'));
    const obj = JSON.parse(record[5]);
    
    if (!obj) {
      trWorkplace.innerHTML = `
        <td class="editable" colspan="4" onclick="timesheetEdit(${record[0]});">
        <div><i class="ti ti-edit"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
        </td>
      `;
    } else if (Object.keys(obj).length === 1) {
      trWorkplace.innerHTML = `
        <td class="editable" colspan="4" onclick="timesheetDelete(${record[0]});">
        <div><i class="ti ti-trash"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
        </td>
      `;
    } else {
      trWorkplace.innerHTML = `
        <td class="editable" colspan="4" onclick="timesheetRequestDelete(${record[0]});">
        <div><i class="ti ti-refresh"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
        </td>
      `;
    }
    tbody.appendChild(trWorkplace);

    if (record[2] !== code.id.NORMAL) {
      const trCode = document.createElement('tr');

      trCode.innerHTML = `<td class="taRight" colspan="4">Clasificación: ${code.name[record[2]]}</td>`;
      tbody.appendChild(trCode);
    }

    const trData = document.createElement('tr');

    trData.innerHTML = `
      <td>${longDate(record[3])}</td>
      <td>${shortTime(record[3])}</td>
      <td>${shortTime(record[4])}</td>
      <td>${timeDiff(dt2, dt1)}</td>
    `;
    tbody.appendChild(trData);

    if (!obj) return;

    const trReason = document.createElement('tr');

    if (Object.keys(obj).length === 1) {

      trReason.innerHTML = `<td class="taLeft" colspan="4">${obj.reason}</td>`;
      tbody.appendChild(trReason);
    } else {
      const dt3 = new Date(obj.clock_in.replace(' ', 'T'));
      const dt4 = new Date(obj.clock_out.replace(' ', 'T'));
      const trModified = document.createElement('tr');
      const trDataModified = document.createElement('tr');

      trModified.innerHTML = `<td class="taLeft" colspan="4">Modificado, ${obj.workplace_name}</td>`;
      trDataModified.innerHTML = `
        <td>${longDate(obj.clock_in)}</td>
        <td>${shortTime(obj.clock_in)}</td>
        <td>${shortTime(obj.clock_out)}</td>
        <td>${timeDiff(dt4, dt3)}</td>
      `;
      trReason.innerHTML = `<td class="taLeft" colspan="4">${obj.reason}</td>`;
      tbody.appendChild(trModified);
      tbody.appendChild(trDataModified);
      tbody.appendChild(trReason);
    }
  });

  const trMore = document.createElement('tr');

  trMore.innerHTML = `
    <td class="editable" colspan="4" onclick="timesheetMore();">
    <div><i class="ti ti-search"></i><span>Ver más fichajes</span></div>
    </td>
  `;
  tbody.appendChild(trMore);
}

const timesheet = {
  hash: 0,
  id: clockingForm.querySelector('input[name="id"]'),
  workplace: clockingForm.querySelector('select[name="workplace"]'),
  user: clockingForm.querySelector('input[name="user"]'),
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

async function timesheetNew() {
  try {
    const response = await fetch('/api/timesheet/empty', {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();
      timesheetEditUI(data);
    } else if (response.status === 204) {
      showMessage('No se puede crear el registro');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP Error ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede crear el registro');
  }
}

async function timesheetEdit(id) {
  try {
    const response = await fetch(`/api/timesheet/${id}`, {
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

  timesheet.user.value = data.user_id;

  let date, time;
  [date, time] = pairDateTime(data.clock_in);
  timesheet.clockIn.date.value = date;
  timesheet.clockIn.time.value = time;
  [date, time] = pairDateTime(data.clock_out);
  timesheet.clockOut.date.value = date;
  timesheet.clockOut.time.value = time;

  timesheet.reason.value = "";

  timesheet.hash = formHash(clockingForm);
  timesheet.clockIn.date.focus();
}

document.querySelectorAll('#clocking-form input').forEach(element => {
  element.addEventListener('blur', function() {
    this.value = this.value.trim();
  });
  element.addEventListener('keydown', function(e) {
    if (e.key === 'Enter' && !this.list) {
      this.value = this.value.trim();
    }
  });
});

clockingForm.addEventListener('submit', (e) => {
  e.preventDefault();

  const hash = formHash(clockingForm);

  if (timesheet.hash === hash) {
    showMessage("No se ha realizado ningún cambio en el registro").then(() => {
      timesheet.clockIn.date.focus();
    });
    return;
  }

  const workplace_id = Number(timesheet.workplace.value);
  const clock_in = `${timesheet.clockIn.date.value} ${timesheet.clockIn.time.value}:00`;
  const clock_out = `${timesheet.clockOut.date.value} ${timesheet.clockOut.time.value}:00`;

  const dt1 = new Date(clock_in.replace(' ', 'T'));
  const dt2 = new Date(clock_out.replace(' ', 'T'));

  if (dt1 >= dt2) {
    showMessage("La hora de entrada no puede ser igual o superior a la de salida").then(() => {
      timesheet.clockIn.date.focus();
    });
    return;
  }
  if (dt2 - dt1 > 9 * 60 * 60 * 1000) {
    showMessage("No pueden transcurrir más de 9 horas entre la hora de entrada y la de salida").then(() => {
      timesheet.clockIn.date.focus();
    });
    return;
  }
  if (user.role !== role.ADMIN) {
    const reason = timesheet.reason.value;

    if (timesheet.id.value == 0) { // Do not compare with ===
      timesheetInsert(JSON.stringify({ workplace_id, clock_in, clock_out, reason }));
    } else {
      const workplace_name = timesheet.workplace.options[timesheet.workplace.selectedIndex].text;
      timesheetRequestUpdate(JSON.stringify({ workplace_id, workplace_name, clock_in, clock_out, reason }));
    }
  } else {
    if (timesheet.id.value == 0) { // Do not compare with ===
      const user_id = Number(timesheet.user.value);
      timesheetInsert(JSON.stringify({ workplace_id, user_id, clock_in, clock_out }));
    } else {
      timesheetUpdate(JSON.stringify({ workplace_id, clock_in, clock_out }));
    }
  }
});

document.getElementById("clocking-cancel").addEventListener("click", () => {
  clockingForm.style.display = "none";
  clockingTable.style.display = "table";
});

async function timesheetUpsert() {
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

async function timesheetInsert(data) {
  try {
    const response = await fetch('/api/timesheet', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      credentials: 'include',
      body: data
    });

    if (response.status === 200) {
      clockingForm.style.display = "none";
      clockingTable.style.display = "table";
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No se puede fichar en las horas indicadas');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede insertar el registro');
  }
}

async function timesheetRequestUpdate(data) {
  try {
    const response = await fetch(`/api/timesheet/${timesheet.id.value}/request`, {
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
      showMessage("No se ha realizado ningún cambio en el registro");
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
    const response = await fetch(`/api/timesheet/${timesheet.id.value}`, {
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
      showMessage('No se puede fichar en las horas indicadas');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede actualizar el registro');
  }
}

async function timesheetRequestDelete(id) {
  if (!await confirmMessage("Se eliminará la solicitud pendiente")) {
    return;
  }
  try {
    const response = await fetch(`/api/timesheet/${id}/request`, {
      method: 'PATCH',
      credentials: 'include'
    });

    if (response.status === 200) {
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

async function timesheetDelete(id) {
  if (!await confirmMessage("Se eliminará el registro seleccionado")) {
    return;
  }
  try {
    const response = await fetch(`/api/timesheet/${id}`, {
      method: 'DELETE',
      credentials: 'include'
    });

    if (response.status === 200) {
      await refreshClockingTable();
    } else if (response.status === 204) {
      showMessage('No hay nada que eliminar');
    } else {
      const text = await response.text();
      showMessage(text || `HTTP ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message || 'No se puede eliminar el registro');
  }
}

async function timesheetSelectMonth(year, month) {
/*
  const nombre = await promptMessage("Por favor ingrese un nombre:", "John Doe");

  if (nombre === null) {
    return;
  }
*/
  try {
    const response = await fetch(`/api/timesheet/${year}/${month}/month`, {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();
      refreshClockingTableUI(data);
    } else if (response.status !== 204) {
      const text = await response.text();
      showMessage(text || `HTTP Error ${response.status}`);
    }
  } catch (error) {
    showMessage(error.message ||  `HTTP ${response.status}`);
  }
}

function timesheetMore() {
  showForm(createMonthYearForm, 'Filtrar fichajes por mes').then(result => {
    if (result) { timesheetSelectMonth(result.year, result.month); }
  });
}

