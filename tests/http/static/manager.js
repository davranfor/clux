const schedule = {
  id: 0, date: null
}

async function refreshScheduleTable(id) {
  const response = await fetch(`/api/users/${id}/schedule`, {
    method: 'GET',
    credentials: 'include'
  });

  if (response.status === 200) {
    const data = await response.json();
    schedule.id = id;
    user.role === role.BASIC ? refreshScheduleTableRead(data) : refreshScheduleTableWrite(data);
  } else if (response.status === 204) {
    throw new Error('El registro ya no existe');
  } else {
    const text = await response.text();
    throw new Error(text || `HTTP Error ${response.status}`);
  }
}

function refreshScheduleTableCommon(object) {
  const today = new Date(object.today);

  schedule.date = sumDays(today, -getISODay(today));

  const parsedObject = JSON.parse(object.schedule);
  const rotateDate = new Date(parsedObject?.date || schedule.date).getTime() !== schedule.date.getTime();
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
}

function refreshScheduleTableRead(object) {
  const data = refreshScheduleTableCommon(object);
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
  if (schedule.id !== user.id) {
    const trUser = document.createElement('tr');

    trUser.innerHTML = `<th colspan="5">${object.name}</th>`;
    tbody.appendChild(trUser);
  }
  for (let i = 0; i < 14; i++) {
    const date = sumDays(schedule.date, i);

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
}

function refreshScheduleTableWrite(object) {
  const data = refreshScheduleTableCommon(object);
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
  if (schedule.id !== user.id) {
    const trUser = document.createElement('tr');

    trUser.innerHTML = `<th colspan="5">${object.name} (${object.workplaces.find(item => item[0] === object.workplace_id)?.[1]})</th>`;
    tbody.appendChild(trUser);
  }
  for (let i = 0; i < 14; i++) {
    const date = sumDays(schedule.date, i);

    if ((i % 7) === 0) {
      const tr0 = document.createElement('tr');

      tr0.innerHTML = '<th>Fecha</th><th colspan="2">Primer turno</th><th colspan="2">Segundo turno</th>';
      tbody.appendChild(tr0);
    }

    const tr1 = document.createElement('tr');
    const tr2 = document.createElement('tr');

    tr1.innerHTML = `
      <th rowspan="2" class="date" onclick="scheduleChange(${i});">${formatDate(date)}<br>${dayOfWeek(date)}</th>
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
}

function scheduleChange(selected) {
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

  scheduleUpdate(values);
});

document.getElementById("schedule-cancel").addEventListener("click", () => {
  menuBack();
});

async function scheduleUpdate(data) {
  try {
    const response = await fetch(`/api/users/${schedule.id}/schedule`, {
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

const profile = {
  hash: 0,
  inserting: false,
  id: profileForm.querySelector('input[name="id"]'),
  workplace: profileForm.querySelector('select[name="workplace"]'),
  category: profileForm.querySelector('select[name="category"]'),
  role: profileForm.querySelector('select[name="role"]'),
  name: profileForm.querySelector('input[name="name"]'),
  tin: profileForm.querySelector('input[name="tin"]'),
  address: profileForm.querySelector('textarea[name="address"]'),
  phone: profileForm.querySelector('input[name="phone"]'),
  email: profileForm.querySelector('input[name="email"]')
}

async function profileEdit(id) {
  profile.inserting = id === 0;

  const response = await fetch(`/api/users/${id}`, {
    method: 'GET',
    credentials: 'include'
  });

  if (response.status === 200) {
    const data = await response.json();
    profile.inserting ? profileInsertUI(data) : profileEditUI(data);
  } else if (response.status === 204) {
    throw new Error('El registro ya no existe');
  } else {
    const text = await response.text();
    throw new Error(text || `HTTP Error ${response.status}`);
  }
}

function profileFocus() {
  if (user.role === role.ADMIN) {
    if (profile.inserting) {
      profile.id.disabled = false;
      profile.id.focus();
    } else {
      profile.id.disabled = true;
      profile.workplace.focus();
    }
  } else {
    profile.name.focus();
    profile.name.select();
  }
}

function profileFillLists(data) {
  profile.workplace.replaceChildren();
  profile.category.replaceChildren();
  if (profile.inserting) {
    const emptyWorkplace = document.createElement('option');

    emptyWorkplace.value = "";
    emptyWorkplace.textContent = "Selecciona un centro de trabajo";
    profile.workplace.appendChild(emptyWorkplace);

    const emptyCategory = document.createElement('option');

    emptyCategory.value = "";
    emptyCategory.textContent = "Selecciona una categoría";
    profile.category.appendChild(emptyCategory);
  }
  data.workplaces.forEach(workplace => {
    const option = document.createElement('option');

    option.value = workplace[0];
    option.textContent = workplace[1];
    profile.workplace.appendChild(option);
  });
  data.categories.forEach(category => {
    const option = document.createElement('option');

    option.value = category[0];
    option.textContent = category[1];
    profile.category.appendChild(option);
  });
}

function profileInsertUI(data) {
  profileTable.style.display = "none";
  profileForm.reset();
  profileFillLists(data);
  profile.hash = formHash(profileForm);
}

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

function profileEditUI(data) {
  if (user.id != data.id) {
    profileShowOptions(data);
    profileTable.style.display = "table";
  } else {
    profileTable.style.display = "none";
  }
  profileFillLists(data);
  profile.id.value = data.id;
  profile.workplace.value = data.workplace_id;
  profile.category.value = data.category_id;
  profile.role.value = data.role;
  profile.name.value = data.name;
  profile.tin.value = data.tin;
  profile.address.value = data.address;
  profile.phone.value = data.phone;
  profile.email.value = data.email;
  profile.hash = formHash(profileForm);
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

profileForm.addEventListener('submit', (e) => {
  e.preventDefault();

  const hash = formHash(profileForm);

  if (profile.hash === hash) {
    showMessage("No se ha realizado ningún cambio en el registro").then(() => {
      profileFocus();
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

    profileUpsert(JSON.stringify({ workplace_id, category_id, role, name, tin, address, phone, email }));
  } else {
    profileUpsert(JSON.stringify({ name, tin, address, phone, email }));
  }
});

document.getElementById("profile-cancel").addEventListener("click", () => {
  menuBack();
});

async function profileUpsert(data) {
  try {
    const response = await fetch(`/api/users/${profile.id.value}`, {
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

const team = {
  view: { MyWorkplace: 0, OtherWorkplace: 1, ListOfWorkplaces: 2 },
  viewIndex: 0,
  selectedWorkplace: { id: 0, name: null }
}

async function refreshTeamTable() {
  let url = null;

  switch (team.viewIndex) {
    case team.view.MyWorkplace:
      url = '/api/users/workplace';
      break;
    case team.view.OtherWorkplace:
      url = `/api/users/${team.selectedWorkplace.id}/workplace`;
      break;
    case team.view.ListOfWorkplaces:
      url = '/api/workplaces';
      break;
  }
  const response = await fetch(url, {
    method: 'GET',
    credentials: 'include'
  });

  if (response.status === 200) {
    const data = await response.json();
    refreshTeamTableUI(data);
  } else if (response.status !== 204) {
    team.viewIndex = team.view.MyWorkplace;
    const text = await response.text();
    throw new Error(text || `HTTP Error ${response.status}`);
  }
}

async function refreshTeamMyWorkplace() {
  team.viewIndex = team.view.MyWorkplace;
  try {
    await refreshTeamTable();
  } catch (error) {
    if (error.message) showMessage(error.message);
  }
}

async function refreshTeamOtherWorkplace(id, name) {
  team.viewIndex = team.view.OtherWorkplace;
  team.selectedWorkplace.id = id;
  team.selectedWorkplace.name = name;
  try {
    await refreshTeamTable();
  } catch (error) {
    if (error.message) showMessage(error.message);
  }
}

async function refreshTeamListOfWorkplaces() {
  team.viewIndex = team.view.ListOfWorkplaces;
  try {
    await refreshTeamTable();
  } catch (error) {
    if (error.message) showMessage(error.message);
  }
}

function refreshTeamTableUI(data) {
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
  if (team.viewIndex === team.view.MyWorkplace) {
    if (user.role === role.ADMIN) {
      const trSelector = document.createElement('tr');

      trSelector.innerHTML = `
        <td class="clickable" colspan="3" onclick="refreshTeamListOfWorkplaces();">
        <div><i class="ti ti-world-longitude"></i><span>Todos los equipos</span></div>
        </td>
      `;
      tbody.appendChild(trSelector);
    }

    const trTitle = document.createElement('tr');

    trTitle.innerHTML = '<th colspan="2">Mi equipo</th><th>Fichajes</th>';
    tbody.appendChild(trTitle);
  } else if (team.viewIndex === team.view.OtherWorkplace) {
    const trMyTeam = document.createElement('tr');
    const trSelector = document.createElement('tr');
    const trTitle = document.createElement('tr');

    trMyTeam.innerHTML = `
      <td class="clickable" colspan="3" onclick="refreshTeamMyWorkplace();">
      <div><i class="ti ti-world-longitude"></i><span>Mi equipo</span></div>
      </td>
    `;
    trSelector.innerHTML = `
      <td class="clickable" colspan="3" onclick="refreshTeamListOfWorkplaces();">
      <div><i class="ti ti-world-longitude"></i><span>Todos los equipos</span></div>
      </td>
    `;
    trTitle.innerHTML = `<th colspan="2">${team.selectedWorkplace.name}</th><th>Fichajes</th>`;
    tbody.appendChild(trMyTeam);
    tbody.appendChild(trSelector);
    tbody.appendChild(trTitle);

  } else { // team.view.ListOfWorkplaces
    const trSelector = document.createElement('tr');
    const trTitle = document.createElement('tr');

    trSelector.innerHTML = `
      <td class="clickable" colspan="3" onclick="refreshTeamMyWorkplace();">
      <div><i class="ti ti-world-longitude"></i><span>Mi equipo</span></div>
      </td>
    `;
    trTitle.innerHTML = '<th colspan="2">Todos los equipos</th><th>Fichajes</th>';
    tbody.appendChild(trSelector);
    tbody.appendChild(trTitle);
  }
  if (team.viewIndex !== team.view.ListOfWorkplaces) {
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
      trWorkplace.addEventListener('click', () => { refreshTeamOtherWorkplace(record[0], record[1]); });
      trWorkplace.innerHTML = `
        <td>${record[0]}</td>
        <td>${record[1]}</td>
        <td>${record[2]}</td>
      `;
      tbody.appendChild(trWorkplace);
    });
  }
}

