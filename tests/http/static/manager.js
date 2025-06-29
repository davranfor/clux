const clocking = {
  frame: document.getElementById("clocking-frame"),

  // ClockIn
  time: null, timeout: null, elapsed: 0,
  text: document.getElementById('clocking-text'),
  redraw() {
    const now = Date.now();

    this.text.textContent = formatTime(now - this.time);
    this.timeout = setTimeout(() => this.redraw(), 1000 - (now % 1000));
  },
  start() {
    if (this.timeout) {
      clearTimeout(this.timeout);
      this.timeout = null;
    }
    this.time = Date.now() - (this.elapsed * 1000);
    this.text.textContent = '00:00:00';
    this.elapsed = 0;
    this.redraw();
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
  },

  async upsert() {
    const response = await fetch('/api/users/clock_in', {
      method: 'PATCH',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();

      user.clockIn = data[0] || 0;
    } else if (response.status === 204) {
      throw new Error('No se puede fichar en este momento');
    } else {
      const text = await response.text();

      throw new Error(text || `HTTP Error ${response.status}`);
    }
  },

  // Table
  table: document.getElementById("clocking-table"),
  title: document.getElementById("clocking-title"),

  async refresh() {
    const response = await fetch('/api/timelogs/week', {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();

      this.refreshTable(data);
    } else if (response.status !== 204) {
      const text = await response.text();

      throw new Error(text || `HTTP Error ${response.status}`);
    }
  },
  async show() {
    await this.refresh();
    if (this.frame.style.display !== "flex") {
      this.frame.style.display = "flex";
    }
    if (this.form.style.display !== "none") {
      this.form.style.display = "none";
    }
    if (this.table.style.display !== "table") {
      this.table.style.display = "table";
    }
  },
  hide() {
    this.frame.style.display = "none";
  },
  refreshTable(data) {
    const tbody = document.querySelector('#clocking-table tbody');

    tbody.replaceChildren();

    const trNew = document.createElement('tr');

    trNew.innerHTML = `
      <td class="clickable" colspan="4" onclick="clocking.add();">
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
          <td class="clickable" colspan="4" onclick="clocking.edit(${record[0]});">
          <div><i class="ti ti-edit"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
          </td>
        `;
      } else if (Object.keys(obj).length === 1) {
        trWorkplace.innerHTML = `
          <td class="clickable" colspan="4" onclick="clocking.delete(${record[0]});">
          <div><i class="ti ti-trash"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
          </td>
        `;
      } else {
        trWorkplace.innerHTML = `
          <td class="clickable" colspan="4" onclick="clocking.requestDelete(${record[0]});">
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
      <td class="clickable" colspan="4" onclick="clocking.more();">
      <div><i class="ti ti-search"></i><span>Ver más fichajes</span></div>
      </td>
    `;
    tbody.appendChild(trMore);
  },

  // Form
  form: document.getElementById("clocking-form"),
  id: document.getElementById("clocking-form").querySelector('input[name="id"]'),
  workplace: document.getElementById("clocking-form").querySelector('select[name="workplace"]'),
  user: document.getElementById("clocking-form").querySelector('input[name="user"]'),
  clockIn: {
    date: document.getElementById("clocking-form").querySelector('input[name="clock_in_date"]'),
    time: document.getElementById("clocking-form").querySelector('input[name="clock_in_time"]'),
  },
  clockOut: {
    date: document.getElementById("clocking-form").querySelector('input[name="clock_out_date"]'),
    time: document.getElementById("clocking-form").querySelector('input[name="clock_out_time"]'),
   },
  reason: document.getElementById("clocking-form").querySelector('input[name="reason"]'),
  hash: 0,

  async add() {
    try {
      const response = await fetch('/api/timelogs/empty', {
        method: 'GET',
        credentials: 'include'
      });

      if (response.status === 200) {
        const data = await response.json();

        this.showForm(data);
      } else if (response.status === 204) {
        showMessage('No se puede crear el registro');
      } else {
        const text = await response.text();

        showMessage(text || `HTTP Error ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede crear el registro');
    }
  },
  async edit(id) {
    try {
      const response = await fetch(`/api/timelogs/${id}`, {
        method: 'GET',
        credentials: 'include'
      });

      if (response.status === 200) {
        const data = await response.json();

        this.showForm(data);
      } else if (response.status === 204) {
        showMessage('El registro ya no existe');
      } else {
        const text = await response.text();

        showMessage(text || `HTTP Error ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede editar el registro');
    }
  },
  showForm(data) {
    this.id.value = data.id;

    this.workplace.replaceChildren();
    data.workplaces.forEach(workplace => {
      const option = document.createElement('option');

      option.value = workplace[0];
      option.textContent = workplace[1];
      this.workplace.appendChild(option);
    });
    this.workplace.value = data.workplace_id;

    this.user.value = data.user_id;

    let date, time;
    [date, time] = pairDateTime(data.clock_in);
    this.clockIn.date.value = date;
    this.clockIn.time.value = time;
    [date, time] = pairDateTime(data.clock_out);
    this.clockOut.date.value = date;
    this.clockOut.time.value = time;

    this.reason.value = "";

    this.hash = formHash(this.form);

    if (this.frame.style.display !== "flex") {
      this.frame.style.display = "flex";
    }
    if (this.table.style.display !== "none") {
      this.table.style.display = "none";
    }
    if (this.form.style.display !== "flex") {
      this.form.style.display = "flex";
    }
    this.clockIn.date.focus();
  },
  async insert(data) {
    try {
      const response = await fetch('/api/timelogs', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: data
      });

      if (response.status === 200) {
        this.show();
      } else if (response.status === 204) {
        showMessage('No se puede fichar en las horas indicadas');
      } else {
        const text = await response.text();

        showMessage(text || `HTTP ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede insertar el registro');
    }
  },
  async requestUpdate(data) {
    try {
      const response = await fetch(`/api/timelogs/${clocking.id.value}/request`, {
        method: 'PATCH',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: data
      });

      if (response.status === 200) {
        this.show();
      } else if (response.status === 204) {
        showMessage("No se ha realizado ningún cambio en el registro");
      } else {
        const text = await response.text();

        showMessage(text || `HTTP ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede actualizar el registro');
    }
  },
  async update(data) {
    try {
      const response = await fetch(`/api/timelogs/${clocking.id.value}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: data
      });

      if (response.status === 200) {
        this.show();
      } else if (response.status === 204) {
        showMessage('No se puede fichar en las horas indicadas');
      } else {
        const text = await response.text();

        showMessage(text || `HTTP ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede actualizar el registro');
    }
  },
  async requestDelete(id) {
    if (!await confirmMessage("Se eliminará la solicitud pendiente")) {
      return;
    }
    try {
      const response = await fetch(`/api/timelogs/${id}/request`, {
        method: 'PATCH',
        credentials: 'include'
      });

      if (response.status === 200) {
        this.show();
      } else if (response.status === 204) {
        showMessage('No hay nada que eliminar');
      } else {
        const text = await response.text();

        showMessage(text || `HTTP ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede actualizar el registro');
    }
  },
  async delete(id) {
    if (!await confirmMessage("Se eliminará el registro seleccionado")) {
      return;
    }
    try {
      const response = await fetch(`/api/timelogs/${id}`, {
        method: 'DELETE',
        credentials: 'include'
      });

      if (response.status === 200) {
        this.show();
      } else if (response.status === 204) {
        showMessage('No hay nada que eliminar');
      } else {
        const text = await response.text();

        showMessage(text || `HTTP ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message || 'No se puede eliminar el registro');
    }
  },
  async selectMonth(year, month) {
  /*
    const nombre = await promptMessage("Por favor ingrese un nombre:", "John Doe");

    if (nombre === null) {
      return;
    }
  */
    try {
      const response = await fetch(`/api/timelogs/${year}/${month}/month`, {
        method: 'GET',
        credentials: 'include'
      });

      if (response.status === 200) {
        const data = await response.json();

        this.refreshTable(data);
      } else if (response.status !== 204) {
        const text = await response.text();

        showMessage(text || `HTTP Error ${response.status}`);
      }
    } catch (error) {
      showMessage(error.message ||  `HTTP ${response.status}`);
    }
  },
  more() {
    showForm(createMonthYearForm, 'Filtrar fichajes por mes').then(result => {
      if (result) { this.selectMonth(result.year, result.month); }
    });
  }
};

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

document.getElementById("clocking-form").addEventListener('submit', (e) => {
  e.preventDefault();

  const hash = formHash(clocking.form);

  if (clocking.hash === hash) {
    showMessage("No se ha realizado ningún cambio en el registro").then(() => {
      clocking.clockIn.date.focus();
    });
    return;
  }

  const workplace_id = Number(clocking.workplace.value);
  const clock_in = `${clocking.clockIn.date.value} ${clocking.clockIn.time.value}:00`;
  const clock_out = `${clocking.clockOut.date.value} ${clocking.clockOut.time.value}:00`;

  const dt1 = new Date(clock_in.replace(' ', 'T'));
  const dt2 = new Date(clock_out.replace(' ', 'T'));

  if (dt1 >= dt2) {
    showMessage("La hora de entrada no puede ser igual o superior a la de salida").then(() => {
      clocking.clockIn.date.focus();
    });
    return;
  }
  if (dt2 - dt1 > 9 * 60 * 60 * 1000) {
    showMessage("No pueden transcurrir más de 9 horas entre la hora de entrada y la de salida").then(() => {
      clocking.clockIn.date.focus();
    });
    return;
  }
  if (user.role !== role.ADMIN) {
    const reason = clocking.reason.value;

    if (clocking.id.value == 0) { // Do not compare with ===
      clocking.insert(JSON.stringify({ workplace_id, clock_in, clock_out, reason }));
    } else {
      const workplace_name = clocking.workplace.options[clocking.workplace.selectedIndex].text;
      clocking.requestUpdate(JSON.stringify({ workplace_id, workplace_name, clock_in, clock_out, reason }));
    }
  } else {
    if (clocking.id.value == 0) { // Do not compare with ===
      const user_id = Number(clocking.user.value);

      clocking.insert(JSON.stringify({ workplace_id, user_id, clock_in, clock_out }));
    } else {
      clocking.update(JSON.stringify({ workplace_id, clock_in, clock_out }));
    }
  }
});

document.getElementById("clocking-cancel").addEventListener("click", () => {
  try { clocking.show(); } catch (error) { showMessage(error.message); }
});

const schedule = {
  frame: document.getElementById("schedule-frame"),
  id: 0, date: null,

  async refresh(id) {
    const response = await fetch(`/api/users/${id}/schedule`, {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();

      this.id = id;
      user.role === role.BASIC ? this.refreshTableForRead(data) : this.refreshTableForWrite(data);
    } else if (response.status === 204) {
      throw new Error('El registro ya no existe');
    } else {
      const text = await response.text();

      throw new Error(text || `HTTP Error ${response.status}`);
    }
  },
  async show(id) {
    await this.refresh(id);
    if (this.frame.style.display !== "flex") {
      this.frame.style.display = "flex";
    }
  },
  hide() {
    this.frame.style.display = "none";
  },
  parse(object) {
    const today = new Date(object.today);

    this.date = sumDays(today, -getISODay(today));

    const parsedObject = JSON.parse(object.schedule);
    const rotateDate = new Date(parsedObject?.date || this.date).getTime() !== this.date.getTime();
    const empty = [[object.workplace_id, '00:00', '00:00'], [0, '00:00', '00:00']];
    let data = parsedObject?.data || null;

    if (!data || data.length !== 14) {
      data = Array.from({ length: 14 }, () => empty);
    }
    if (rotateDate) {
      //showMessage("Ha rotado");
      for (let i = 7; i < 14; i++) {
        data[i - 7] = JSON.parse(JSON.stringify(data[i]));
        data[i] = empty;
      }
    }
    return data;
  },
  refreshTableForRead(object) {
    const data = this.parse(object);
    const workplaces = object.workplaces;
    const workplacesMap = {
      0: "Sin centro asignado",
      ...workplaces.reduce((map, workplace) => {
        map[workplace[0]] = workplace[1];
        return map;
      }, {})
    };
    const tbody = document.querySelector('#schedule-table tbody');

    tbody.replaceChildren();
    if (this.id !== user.id) {
      const trUser = document.createElement('tr');

      trUser.innerHTML = `<th colspan="5">${object.name}</th>`;
      tbody.appendChild(trUser);
    }
    for (let i = 0; i < 14; i++) {
      const date = sumDays(this.date, i);

      if ((i % 7) === 0) {
        const tr0 = document.createElement('tr');

        tr0.innerHTML = '<th>Fecha</th><th colspan="2">Primer turno</th><th colspan="2">Segundo turno</th>';
        tbody.appendChild(tr0);
      }

      const tr1 = document.createElement('tr');
      const tr2 = document.createElement('tr');

      tr1.innerHTML = `
        <th rowspan="2">${formatDate(date)}<br>${dayOfWeek(date)}</th>
        <td colspan="2" class="taLeft">${workplacesMap[data[i][0][0]]}</td>
        <td colspan="2" class="taLeft">${workplacesMap[data[i][1][0]]}</td>
      `;
      tr2.innerHTML = `
        <td>${data[i][0][1]}</td>
        <td>${data[i][0][2]}</td>
        <td>${data[i][1][1]}</td>
        <td>${data[i][1][2]}</td>
      `;
      tbody.appendChild(tr1);
      tbody.appendChild(tr2);
    }
  },
  refreshTableForWrite(object) {
    const data = this.parse(object);
    const workplaces = object.workplaces;
    const options = [
      '<option value="0">Selecciona un centro</option>',
      ...workplaces.map(t => `<option value="${t[0]}">${t[1]}</option>`)
    ].join('');

    function createSelect(value) {
      const select = document.createElement('select');

      select.innerHTML = options;
      select.value = value;
      return select;
    }

    const tbody = document.querySelector('#schedule-table tbody');

    tbody.replaceChildren();
    if (this.id !== user.id) {
      const trUser = document.createElement('tr');

      trUser.innerHTML = `<th colspan="5">${object.name} (${object.workplaces.find(item => item[0] === object.workplace_id)?.[1]})</th>`;
      tbody.appendChild(trUser);
    }
    for (let i = 0; i < 14; i++) {
      const date = sumDays(this.date, i);

      if ((i % 7) === 0) {
        const tr0 = document.createElement('tr');

        tr0.innerHTML = '<th>Fecha</th><th colspan="2">Primer turno</th><th colspan="2">Segundo turno</th>';
        tbody.appendChild(tr0);
      }

      const tr1 = document.createElement('tr');
      const tr2 = document.createElement('tr');

      tr1.innerHTML = `
        <th rowspan="2" class="date" onclick="schedule.change(${i});">${formatDate(date)}<br>${dayOfWeek(date)}</th>
        <td colspan="2" class="entry"></td>
        <td colspan="2" class="entry"></td>
      `;

      const [select1, select2] = tr1.querySelectorAll('.entry');

      select1.appendChild(createSelect(data[i][0][0]));
      select2.appendChild(createSelect(data[i][1][0]));

      tr2.innerHTML = `
        <td class="entry"><input type="time" maxLength="5" value="${data[i][0][1]}"></td>
        <td class="entry"><input type="time" maxLength="5" value="${data[i][0][2]}"></td>
        <td class="entry"><input type="time" maxLength="5" value="${data[i][1][1]}"></td>
        <td class="entry"><input type="time" maxLength="5" value="${data[i][1][2]}"></td>
      `;
      tbody.appendChild(tr1);
      tbody.appendChild(tr2);
    }
  },
  change(selected) {
  /*
    confirmMessage("Se clonarán los datos a los registros con fecha posterior en la semana seleccionada").then((confirmed) => {
      if (!confirmed) return;
    });
  */
    const rows = document.querySelectorAll('#schedule-table tr');
    let data = [[], []];
    let index = 0;

    for (const tr of rows) {
      if (tr.querySelector('td.entry')) {

        const entries = tr.querySelectorAll('td.entry');

        if (entries.length === 2) {
          if (index === selected) {
            data[0][0] = Number(entries[0].querySelector('select')?.value || '0');
            data[1][0] = Number(entries[1].querySelector('select')?.value || '0');
          } else if (index > selected) {
            entries[0].querySelector('select').value = data[0][0];
            entries[1].querySelector('select').value = data[1][0];
          }
        } else if (entries.length === 4) {
          if (index === selected) {
            data[0][1] = entries[0].querySelector('input[type="time"]')?.value || '00:00';
            data[0][2] = entries[1].querySelector('input[type="time"]')?.value || '00:00';
            data[1][1] = entries[2].querySelector('input[type="time"]')?.value || '00:00';
            data[1][2] = entries[3].querySelector('input[type="time"]')?.value || '00:00';
          } else if (index > selected) {
            entries[0].querySelector('input[type="time"]').value = data[0][1] ;
            entries[1].querySelector('input[type="time"]').value = data[0][2] ;
            entries[2].querySelector('input[type="time"]').value = data[1][1] ;
            entries[3].querySelector('input[type="time"]').value = data[1][2] ;
          }
          if (++index >= selected - (selected % 7) + 7) return;
        }
      }
    }
  },
  async update(data) {
    try {
      const response = await fetch(`/api/users/${this.id}/schedule`, {
        method: 'PATCH',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: data
      });

      if (response.status === 200) {
        await response.text();
        menuBack();
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
}

document.getElementById("schedule-submit").addEventListener("click", () => {
  const rows = document.querySelectorAll('#schedule-table tr');
  const data = Array.from({ length: 14 }, () => [[0, '00:00', '00:00'], [0, '00:00', '00:00']]);
  let index = 0;

  rows.forEach(tr => {
    if (tr.querySelector('td.entry')) {
      const entries = tr.querySelectorAll('td.entry');

      if (entries.length === 2) {
        data[index][0][0] = Number(entries[0].querySelector('select')?.value || '0');
        data[index][1][0] = Number(entries[1].querySelector('select')?.value || '0');
      } else if (entries.length === 4) {
        data[index][0][1] = entries[0].querySelector('input[type="time"]')?.value || '00:00';
        data[index][0][2] = entries[1].querySelector('input[type="time"]')?.value || '00:00';
        data[index][1][1] = entries[2].querySelector('input[type="time"]')?.value || '00:00';
        data[index][1][2] = entries[3].querySelector('input[type="time"]')?.value || '00:00';
        index++;
      }
    }
  });

  const object = { date: formatISODate(schedule.date), data: data };
  //const object = { date: "2025-06-16", data: data };
  //const object = { date: "2025-06-23", data: data };
  const values = JSON.stringify({ schedule: JSON.stringify(object) });

  schedule.update(values);
});

document.getElementById("schedule-cancel").addEventListener("click", () => {
  menuBack();
});

const profile = {
  frame: document.getElementById("profile-frame"),
  form: document.getElementById("profile-form"),
  id: document.getElementById("profile-form").querySelector('input[name="id"]'),
  workplace: document.getElementById("profile-form").querySelector('select[name="workplace"]'),
  category: document.getElementById("profile-form").querySelector('select[name="category"]'),
  role: document.getElementById("profile-form").querySelector('select[name="role"]'),
  name: document.getElementById("profile-form").querySelector('input[name="name"]'),
  tin: document.getElementById("profile-form").querySelector('input[name="tin"]'),
  address: document.getElementById("profile-form").querySelector('textarea[name="address"]'),
  phone: document.getElementById("profile-form").querySelector('input[name="phone"]'),
  email: document.getElementById("profile-form").querySelector('input[name="email"]'),
  hash: 0,
  inserting: false,

  async show(id) {
    this.inserting = id === 0;

    const response = await fetch(`/api/users/${id}`, {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();

      this.inserting ? this.add(data) : this.edit(data);
    } else if (response.status === 204) {
      throw new Error('El registro ya no existe');
    } else {
      const text = await response.text();

      throw new Error(text || `HTTP Error ${response.status}`);
    }
  },
  hide() {
    this.frame.style.display = "none";
  },
  focus() {
    if (this.frame.style.display !== "flex") {
      this.frame.style.display = "flex";
    }
    if (user.role === role.ADMIN) {
      if (this.inserting) {
        this.id.disabled = false;
        this.id.focus();
      } else {
        this.id.disabled = true;
        this.workplace.focus();
      }
    } else {
      this.name.focus();
      this.name.select();
    }
  },
  fillLists(data) {
    this.workplace.replaceChildren();
    this.category.replaceChildren();
    if (this.inserting) {
      const emptyWorkplace = document.createElement('option');

      emptyWorkplace.value = "";
      emptyWorkplace.textContent = "Selecciona un centro de trabajo";
      this.workplace.appendChild(emptyWorkplace);

      const emptyCategory = document.createElement('option');

      emptyCategory.value = "";
      emptyCategory.textContent = "Selecciona una categoría";
      this.category.appendChild(emptyCategory);
    }
    data.workplaces.forEach(workplace => {
      const option = document.createElement('option');

      option.value = workplace[0];
      option.textContent = workplace[1];
      this.workplace.appendChild(option);
    });
    data.categories.forEach(category => {
      const option = document.createElement('option');

      option.value = category[0];
      option.textContent = category[1];
      this.category.appendChild(option);
    });
  },
  add(data) {
    profileTable.style.display = "none";
    this.form.reset();
    this.fillLists(data);
    this.hash = formHash(this.form);
    this.focus();
  },
  edit(data) {
    if (user.id != data.id) {
      profileShowOptions(data);
      profileTable.style.display = "table";
    } else {
      profileTable.style.display = "none";
    }
    this.fillLists(data);
    this.id.value = data.id;
    this.workplace.value = data.workplace_id;
    this.category.value = data.category_id;
    this.role.value = data.role;
    this.name.value = data.name;
    this.tin.value = data.tin;
    this.address.value = data.address;
    this.phone.value = data.phone;
    this.email.value = data.email;
    this.hash = formHash(this.form);
    this.focus();
  },
  async upsert(data) {
    try {
      const response = await fetch(`/api/users/${this.id.value}`, {
        method: profile.inserting ? 'POST' : 'PUT',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: data
      });

      if (response.status === 200) {
        await response.text();
        menuBack();
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
}

document.querySelectorAll('#profile-form input').forEach(element => {
  element.addEventListener('blur', function() {
    this.value = this.value.trim();
  });
  element.addEventListener('keydown', function(e) {
    if (e.key === 'Enter') {
      this.value = this.value.trim();
    }
  });
});

document.querySelectorAll('#profile-form textarea').forEach(element => {
  element.addEventListener('blur', function() {
    this.value = this.value.trim();
  });
});

document.getElementById("profile-form").addEventListener('submit', (e) => {
  e.preventDefault();

  const hash = formHash(profile.form);

  if (profile.hash === hash) {
    showMessage("No se ha realizado ningún cambio en el registro").then(() => {
      profile.focus();
    });
    return;
  }

  const name = profile.name.value;
  const tin = profile.tin.value;
  const address = profile.address.value;
  const phone = profile.phone.value;
  const email = profile.email.value;

  if (user.role === role.ADMIN) {
    const workplace_id = Number(profile.workplace.value);
    const category_id = Number(profile.category.value);
    const role = Number(profile.role.value);

    profile.upsert(JSON.stringify({ workplace_id, category_id, role, name, tin, address, phone, email }));
  } else {
    profile.upsert(JSON.stringify({ name, tin, address, phone, email }));
  }
});

document.getElementById("profile-cancel").addEventListener("click", () => {
  menuBack();
});

function profileShowOptions(data) {
  const tbody = document.querySelector('#profile-table tbody');

  tbody.replaceChildren();

  const trUser = document.createElement('tr');
  const trData = document.createElement('tr');

  trUser.innerHTML = `<th colspan="5">${data.name} (${data.workplaces.find(item => item[0] === data.workplace_id)?.[1]})</th>`;
  trData.innerHTML = `
    <td class="clickable" onclick="setActiveByKey('item-schedule',${data.id});">
    <div><i class="ti ti-clock"></i><span>Fichajes</span></div>
    </td>
    <td class="clickable" onclick="setActiveByKey('item-schedule',${data.id});">
    <div><i class="ti ti-calendar-time"></i><span>Horario</span></div>
    </td>
    <td class="clickable" onclick="setActiveByKey('item-schedule',${data.id});">
    <div><i class="ti ti-checklist"></i><span>Tareas</span></div>
    </td>
    <td class="clickable" onclick="setActiveByKey('item-schedule',${data.id});">
    <div><i class="ti ti-report"></i><span>Informes</span></div>
    </td>
    <td class="clickable" onclick="setActiveByKey('item-schedule',${data.id});">
    <div><i class="ti ti-trash"></i><span>Eliminar</span></div>
    </td>
  `;
  tbody.appendChild(trUser);
  tbody.appendChild(trData);
}

const team = {
  frame: document.getElementById("team-frame"),
  view: { MyWorkplace: 0, OtherWorkplace: 1, ListOfWorkplaces: 2 },
  viewIndex: 0,
  selectedWorkplace: { id: 0, name: null },

  async show() {
    let url = null;

    switch (this.viewIndex) {
      case this.view.MyWorkplace:
        url = '/api/users/workplace';
        break;
      case this.view.OtherWorkplace:
        url = `/api/users/${this.selectedWorkplace.id}/workplace`;
        break;
      case this.view.ListOfWorkplaces:
        url = '/api/workplaces';
        break;
    }
    const response = await fetch(url, {
      method: 'GET',
      credentials: 'include'
    });

    if (response.status === 200) {
      const data = await response.json();

      this.refreshTable(data);
    } else if (response.status !== 204) {
      this.viewIndex = this.view.MyWorkplace;

      const text = await response.text();

      throw new Error(text || `HTTP Error ${response.status}`);
    }
  },
  hide() {
    this.frame.style.display = "none";
  },
  showMyWorkplace() {
    this.viewIndex = this.view.MyWorkplace;
    try { this.show(); } catch (error) { showMessage(error.message); }
   },
  showOtherWorkplace(id, name) {
    this.viewIndex = this.view.OtherWorkplace;
    this.selectedWorkplace.id = id;
    this.selectedWorkplace.name = name;
    try { this.show(); } catch (error) { showMessage(error.message); }
  },
  showListOfWorkplaces() {
    this.viewIndex = this.view.ListOfWorkplaces;
    try { this.show(); } catch (error) { showMessage(error.message); }
  },
  refreshTable(data) {
    const tbody = document.querySelector('#team-table tbody');

    tbody.replaceChildren();

    if (user.role === role.ADMIN) {
      const trNew = document.createElement('tr');

      trNew.innerHTML = `
        <td class="clickable" colspan="3" onclick="setActiveByKey('item-profile',0);">
        <div><i class="ti ti-user"></i><span>Nuevo usuario</span></div>
        </td>
      `;
      tbody.appendChild(trNew);
    }
    if (this.viewIndex === this.view.MyWorkplace) {
      if (user.role === role.ADMIN) {
        const trSelector = document.createElement('tr');

        trSelector.innerHTML = `
          <td class="clickable" colspan="3" onclick="team.showListOfWorkplaces();">
          <div><i class="ti ti-world-longitude"></i><span>Todos los equipos</span></div>
          </td>
        `;
        tbody.appendChild(trSelector);
      }

      const trTitle = document.createElement('tr');

      trTitle.innerHTML = '<th colspan="2">Mi equipo</th><th>Fichajes</th>';
      tbody.appendChild(trTitle);
    } else if (this.viewIndex === this.view.OtherWorkplace) {
      const trMyTeam = document.createElement('tr');
      const trSelector = document.createElement('tr');
      const trTitle = document.createElement('tr');

      trMyTeam.innerHTML = `
        <td class="clickable" colspan="3" onclick="team.showMyWorkplace();">
        <div><i class="ti ti-world-longitude"></i><span>Mi equipo</span></div>
        </td>
      `;
      trSelector.innerHTML = `
        <td class="clickable" colspan="3" onclick="team.showListOfWorkplaces();">
        <div><i class="ti ti-world-longitude"></i><span>Todos los equipos</span></div>
        </td>
      `;
      trTitle.innerHTML = `<th colspan="2">${this.selectedWorkplace.name}</th><th>Fichajes</th>`;
      tbody.appendChild(trMyTeam);
      tbody.appendChild(trSelector);
      tbody.appendChild(trTitle);

    } else { // this.view.ListOfWorkplaces
      const trSelector = document.createElement('tr');
      const trTitle = document.createElement('tr');

      trSelector.innerHTML = `
        <td class="clickable" colspan="3" onclick="this.showMyWorkplace();">
        <div><i class="ti ti-world-longitude"></i><span>Mi equipo</span></div>
        </td>
      `;
      trTitle.innerHTML = '<th colspan="2">Todos los equipos</th><th>Fichajes</th>';
      tbody.appendChild(trSelector);
      tbody.appendChild(trTitle);
    }
    if (this.viewIndex !== this.view.ListOfWorkplaces) {
      // Sort by category ASC
      data.sort((a, b) => {
        if (a[1] > b[1]) return 1;
        if (a[1] < b[1]) return -1;
        return a[2] > b[2] ? 1 : a[2] < b[2] ? -1 : 0;
      });
      data.forEach(record => {
        const clockIn = record[3] === null ? '' : longDateTime(record[3]);
        const trUser = document.createElement('tr');

        trUser.className = 'team-data';
        trUser.addEventListener('click', () => { setActiveByKey('item-profile', record[0]); });
        trUser.innerHTML = `
          <td>${record[1]}</td>
          <td>${record[2]}</td>
          <td>${clockIn}</td>
        `;
        tbody.appendChild(trUser);
      });
    } else {
      // Sort by name ASC
      data.sort((a, b) => {
        if (a[1] > b[1]) return 1;
        if (a[1] < b[1]) return -1;
        return 0;
      });
      data.forEach(record => {
        const trWorkplace = document.createElement('tr');

        trWorkplace.className = 'team-data';
        trWorkplace.addEventListener('click', () => { this.showOtherWorkplace(record[0], record[1]); });
        trWorkplace.innerHTML = `
          <td>${record[0]}</td>
          <td>${record[1]}</td>
          <td>${record[2]}</td>
        `;
        tbody.appendChild(trWorkplace);
      });
    }
    if (this.frame.style.display !== "flex") {
      this.frame.style.display = "flex";
    }
  }
}

