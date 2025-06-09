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
    try {
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
    } catch (error) {
        throw new Error(error.message || 'Unhandled error');
    }
}

async function refreshClockingTable() {
    try {
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
    } catch (error) {
        throw new Error(error.message || 'Unhandled error');
    }
}

function refreshClockingTableUI(data) {
    const tbody = document.querySelector('#clocking-table tbody');

    tbody.replaceChildren();
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
                <td class="editable" colspan="4" onclick="timesheetClear(${record[0]}, true);">
                <div><i class="ti ti-refresh"></i><span>Modificado, ${obj.workplace_name}</span></div>
                </td>
            `;
            tr4.innerHTML = `
                <td>${longDate(obj.clock_in)}</td>
                <td>${shortTime(obj.clock_in)}</td>
                <td>${shortTime(obj.clock_out)}</td>
                <td>${timeDiff(dt4, dt3)}</td>
            `;
            tr5.innerHTML = `<td class="text-align-left" colspan="4">&nbsp;${obj.reason}</td>`;
            tbody.appendChild(tr3);
            tbody.appendChild(tr4);
            tbody.appendChild(tr5);
        } else {
            const tr3 = document.createElement('tr');
            const tr4 = document.createElement('tr');

            tr3.innerHTML = `
                <td class="editable" colspan="4" onclick="timesheetClear(${record[0]}, true);">
                <div><i class="ti ti-trash"></i><span>Pendiente de eliminar</span></div>
                </td>
            `;
            tr4.innerHTML = `<td class="text-align-left" colspan="4">&nbsp;${obj.reason}</td>`;
            tbody.appendChild(tr3);
            tbody.appendChild(tr4);
        }
    });
}

const timesheet = {
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
            throw new Error(text || `HTTP Error ${response.status}`);
        }
    } catch (error) {
        showMessage(error.message || 'No se puede editar el registro');
    }
}

function timesheetEditUI(data) {
    clockingTable.style.display = "none";
    clockingForm.style.display = "flex";

    timesheet.id.value = data.id;

    timesheet.workplace.innerHTML = '';
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
}

timesheet.reason.addEventListener("blur", function() {
    this.value = this.value.trim();
});

clockingForm.addEventListener('submit', async (e) => {
    e.preventDefault();

    const focusedField = document.activeElement;

    if (focusedField && focusedField.tagName === 'INPUT' && clockingForm.contains(focusedField)) {
        focusedField.blur();
    }

    const workplace_id = Number(timesheet.workplace.value);
    const workplace_name = timesheet.workplace.options[timesheet.workplace.selectedIndex].text;
    const clock_in = `${timesheet.clockIn.date.value} ${timesheet.clockIn.time.value}:00`;
    const clock_out = `${timesheet.clockOut.date.value} ${timesheet.clockOut.time.value}:00`;
    const reason = timesheet.reason.value;

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
    try {
        const response = await fetch(`/api/timesheet/${timesheet.id.value}/change`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            credentials: 'include',
            body: JSON.stringify({ workplace_id, workplace_name, clock_in, clock_out, reason })
        });

        if (response.status === 200) {
            clockingForm.style.display = "none";
            clockingTable.style.display = "table";
            await refreshClockingTable();
        } else if (response.status === 204) {
            throw new Error('No hay ningún cambio que solicitar');
        } else {
            const data = await response.text();
            throw new Error(data || `HTTP ${response.status}`);
        }
    } catch (error) {
        showMessage(error.message || 'Undhandled error');
    }
});

document.getElementById("clocking-delete").addEventListener("click", async () => {
    const reason = timesheet.reason.value;

    if (reason === "") {
        showMessage("Debe especificar un motivo para dar de baja el registro").then(() => {
            timesheet.reason.focus();
        });
        return;
    }
    try {
        const response = await fetch(`/api/timesheet/${timesheet.id.value}/delete`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            credentials: 'include',
            body: JSON.stringify({ reason })
        });

        if (response.status === 200) {
            clockingForm.style.display = "none";
            clockingTable.style.display = "table";
            await refreshClockingTable();
        } else if (response.status === 204) {
            throw new Error('No hay nada que eliminar');
        } else {
            const data = await response.text();
            throw new Error(data || `HTTP ${response.status}`);
        }
    } catch (error) {
        showMessage(error.message || 'Undhandled error');
    }
});

document.getElementById("clocking-cancel").addEventListener("click", () => {
    clockingForm.style.display = "none";
    clockingTable.style.display = "table";
});

async function timesheetClear(id, askConfirm) {
    if (askConfirm) {
        const confirmed = await confirmMessage("Se eliminará la solicitud pendiente");

        if (!confirmed) return;
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
            throw new Error('No hay nada que actualizar');
        } else {
            const data = await response.text();
            throw new Error(data || `HTTP ${response.status}`);
        }
    } catch (error) {
        showMessage(error.message || 'Undhandled error');
    }
}

