const clocking = {
    time: null, start: null, timeout: 0,
    running: false
};

function updateChrono() {
    clockingTime.textContent = formatTime(Date.now() - clocking.start);
    const msToNextSecond = 1000 - (Date.now() % 1000);
    clocking.timeout = setTimeout(updateChrono, msToNextSecond);
}

function startClocking(target, time = null) {
    if (clocking.timeout) clearTimeout(clocking.timeout);

    if (time) {
        const dateParts = time.split(/[- :]/);
        const dateObj = new Date(
            parseInt(dateParts[0]),     // year
            parseInt(dateParts[1]) - 1, // month (0-11)
            parseInt(dateParts[2]),     // day
            parseInt(dateParts[3]),     // hours
            parseInt(dateParts[4]),     // minutes
            parseInt(dateParts[5])      // seconds
        );

        clocking.start = dateObj.getTime();
    } else {
        clocking.start = Date.now();
    }
    target.textContent = '00:00:00';
    clocking.running = true;
    updateChrono();
}

function stopClocking(target) {
    if (clocking.running) {
        clearTimeout(clocking.timeout);
        target.textContent = '';
        clocking.running = false;
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
            user.stationName = data[0];
            user.clockInTime = data[2] === 0 ? data[1] : 0;
        } else if (response.status === 204) {
            throw new Error('Deben transcurrir al menos 5 segundos entre fichajes');
        } else {
            const text = await response.text();
            throw new Error(text || `HTTP Error ${response.status}`);
        }
    } catch (error) {
        throw new Error(error.message || 'Unhandled error');
    }
}

async function showWeek(target) {
    try {
        const response = await fetch('/api/timesheet/week', {
            method: 'GET',
            credentials: 'include'
        });

        if (response.status === 200) {
            const data = await response.json();
            target.textContent = JSON.stringify(data, null, 2) || '';
        } else if (response.status === 204) {
            target.textContent = '';
        } else {
            const text = await response.text();
            throw new Error(text || `HTTP Error ${response.status}`);
        }
    } catch (error) {
        throw new Error(error.message || 'Unhandled error');
    }
}

