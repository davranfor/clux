const clocking = {
    time: null, timeout: null,
    text: document.getElementById('clocking-text'),
    update() {
        const now = Date.now();

        this.text.textContent = formatTime(now - this.time);
        this.timeout = setTimeout(() => this.update(), 1000 - (now % 1000));
    },
    start(time) {
        const now = Date.now();

        if (this.timeout) {
            clearTimeout(this.timeout);
            this.timeout = null;
        }
        this.time = time > now ? now : time;
        this.text.textContent = '00:00:00';
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
    }
}

async function getClockIn() {
    try {
        const response = await fetch('/api/timesheet', {
            method: 'GET',
            credentials: 'include'
        });

        if (response.status === 200) {
            const data = await response.json();
            return data;
        } else if (response.status === 204) {
            return ["", 0];
        } else {
            const text = await response.text();
            throw new Error(text || `HTTP Error ${response.status}`);
        }
    } catch (error) {
        throw new Error(error.message || 'Unhandled error');
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

async function showWeek() {
    try {
        const response = await fetch('/api/timesheet/week', {
            method: 'GET',
            credentials: 'include'
        });

        if (response.status === 200) {
            const data = await response.json();
            showWeekUI(data);
        } else if (response.status !== 204) {
            const text = await response.text();
            throw new Error(text || `HTTP Error ${response.status}`);
        }
    } catch (error) {
        throw new Error(error.message || 'Unhandled error');
    }
}

function showWeekUI(data) {
    const tbody = document.querySelector('#clocking-table tbody');

    tbody.replaceChildren();
    data.reverse();
    data.forEach(record => {
        const dt1 = new Date(record[2].replace(' ', 'T'));
        const dt2 = new Date(record[3].replace(' ', 'T'));
        const tr1 = document.createElement('tr');
        const tr2 = document.createElement('tr');

        tr1.innerHTML = `
            <td class="workday" colspan="4" onclick="timesheetEdit(${record[0]});">
            <div><i class="ti ti-edit"></i><span>${dayOfWeek(dt1)}, ${record[1]}</span></div>
            </td>
        `;
        tr2.innerHTML = `
            <td>${longDate(record[2])}</td>
            <td>${longTime(record[2])}</td>
            <td>${longTime(record[3])}</td>
            <td>${timeDiff(dt2, dt1)}</td>
        `;
        tbody.appendChild(tr1);
        tbody.appendChild(tr2);

        if (record[4] !== null) {
            const obj = JSON.parse(record[4]);
            const dt3 = new Date(obj.clock_in.replace(' ', 'T'));
            const dt4 = new Date(obj.clock_out.replace(' ', 'T'));
            const tr3 = document.createElement('tr');
            const tr4 = document.createElement('tr');

            tr3.innerHTML = `
                <td class="workday" colspan="4" onclick="showMessage('Pendiente de aprobación');">
                <div><i class="ti ti-refresh"></i><span>${dayOfWeek(dt3)}, ${obj.workplace_name}</span></div>
                </td>
            `;
            tr4.innerHTML = `
                <td>${longDate(obj.clock_in)}</td>
                <td>${longTime(obj.clock_in)}</td>
                <td>${longTime(obj.clock_out)}</td>
                <td>${timeDiff(dt4, dt3)}</td>
            `;
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
    }
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
        showMessage(error.message || 'Unhandled error');
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
    [date, time] = roundDateTime(data.clock_in);
    timesheet.clockIn.date.value = date;
    timesheet.clockIn.time.value = time;
    [date, time] = roundDateTime(data.clock_out);
    timesheet.clockOut.date.value = date;
    timesheet.clockOut.time.value = time;
}

[timesheet.clockIn.date, timesheet.clockIn.time,
 timesheet.clockOut.date, timesheet.clockOut.time].forEach(field => {
    field.addEventListener('change', () => {
        timesheet.clockIn.date.setCustomValidity("");
    });
});

clockingForm.addEventListener('submit', async (e) => {
    e.preventDefault();

    const workplace_id = Number(timesheet.workplace.value);
    const workplace_name = timesheet.workplace.options[timesheet.workplace.selectedIndex].text;
    const clock_in = `${timesheet.clockIn.date.value} ${timesheet.clockIn.time.value}:00`;
    const clock_out = `${timesheet.clockOut.date.value} ${timesheet.clockOut.time.value}:00`;

    const dt1 = new Date(clock_in.replace(' ', 'T'));
    const dt2 = new Date(clock_out.replace(' ', 'T'));

    if (dt1 >= dt2) {
        timesheet.clockIn.date.setCustomValidity("La fecha de entrada no puede ser igual o superior a la de salida");
        timesheet.clockIn.date.reportValidity();
        return;
    }
    if (dt2 - dt1 > 9 * 60 * 60 * 1000) {
        timesheet.clockIn.date.setCustomValidity("No pueden transcurrir más de 9 horas entre la fecha de entrada y la de salida");
        timesheet.clockIn.date.reportValidity();
        return;
    }

    const state = `${JSON.stringify({ workplace_id, workplace_name, clock_in, clock_out })}`;

    try {
        const response = await fetch(`/api/timesheet/${timesheet.id.value}/change`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            credentials: 'include',
            body: JSON.stringify({ state })
        });

        if (response.status === 200) {
            clockingForm.style.display = "none";
            clockingTable.style.display = "table";
            await showWeek();
        } else {
            const data = await response.text();
            throw new Error(data || 'Unhandled error');
        }
    } catch (error) {
        showMessage(error.message || 'Undhandled error');
    }
});

document.getElementById("clocking-cancel").addEventListener("click", () => {
    timesheet.clockIn.date.setCustomValidity("");
    clockingForm.style.display = "none";
    clockingTable.style.display = "table";
});

