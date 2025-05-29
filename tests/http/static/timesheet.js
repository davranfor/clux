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
            user.clockInTime = data[2] === 0 ? data[1] : 0;
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

    const select = clockingForm.querySelector('select[name="workplace_id"]');
    select.innerHTML = '';
    data.workplaces.forEach(workplace => {
        const option = document.createElement('option');

        option.value = workplace[0];
        option.textContent = workplace[1];
        select.appendChild(option);
    });
    select.value = data.workplace_id;

    let date, time;
    [date, time] = data.clock_in.split(" ");
    clockingForm.querySelector('input[name="clock_in_date"]').value = date;
    clockingForm.querySelector('input[name="clock_in_time"]').value = time.substring(0, 5);
    [date, time] = data.clock_out.split(" ");
    clockingForm.querySelector('input[name="clock_out_date"]').value = date;
    clockingForm.querySelector('input[name="clock_out_time"]').value = time.substring(0, 5);
}

document.getElementById("clocking-cancel").addEventListener("click", () => {
    clockingForm.style.display = "none";
    clockingTable.style.display = "table";
});

