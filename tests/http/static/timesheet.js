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
            <td>${longDateTime(record[2]).split(' ')[0]}<br></td>
            <td>${record[2].split(' ')[1]}</td>
            <td>${record[3].split(' ')[1]}</td>
            <td>${timeDiff(dt2, dt1)}</td>
        `;
        tbody.appendChild(tr1);
        tbody.appendChild(tr2);
    });
}

const timesheet = {
    id: clockingForm.querySelector('input[name="id"]'),
    workplaceId: clockingForm.querySelector('select[name="workplace_id"]'),
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

    timesheet.workplaceId.innerHTML = '';
    data.workplaces.forEach(workplace => {
        const option = document.createElement('option');

        option.value = workplace[0];
        option.textContent = workplace[1];
        timesheet.workplaceId.appendChild(option);
    });
    timesheet.workplaceId.value = data.workplace_id;

    let date, time;
    [date, time] = data.clock_in.split(" ");
    timesheet.clockIn.date.value = date;
    timesheet.clockIn.time.value = time.substring(0, 5);
    [date, time] = data.clock_out.split(" ");
    timesheet.clockOut.date.value = date;
    timesheet.clockOut.time.value = time.substring(0, 5);
}

[timesheet.clockIn.date, timesheet.clockIn.time,
 timesheet.clockOut.date, timesheet.clockOut.time].forEach(field => {
    field.addEventListener('change', () => {
        timesheet.clockIn.date.setCustomValidity("");
    });
});

clockingForm.addEventListener('submit', async (e) => {
    e.preventDefault();

    const workplace_id = Number(timesheet.workplaceId.value);
    const clock_in = `${timesheet.clockIn.date.value} ${timesheet.clockIn.time.value}:00`;
    const clock_out = `${timesheet.clockOut.date.value} ${timesheet.clockOut.time.value}:00`;

    const dt1 = new Date(clock_in.replace(' ', 'T'));
    const dt2 = new Date(clock_out.replace(' ', 'T'));

    if (dt1 >= dt2) {
        timesheet.clockIn.date.setCustomValidity("La fecha de entrada no puede ser igual o superior a la de salida");
        timesheet.clockIn.date.reportValidity();
        return;
    }
    if (dt2 - dt1 > 12 * 60 * 60 * 1000) {
        timesheet.clockIn.date.setCustomValidity("No pueden transcurrir más de 12 horas entre la fecha de entrada y la de salida");
        timesheet.clockIn.date.reportValidity();
        return;
    }
    try {
        const response = await fetch(`/api/timesheet/${timesheet.id.value}/save`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            credentials: 'include',
            body: JSON.stringify({ workplace_id, clock_in, clock_out })
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

