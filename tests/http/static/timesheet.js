const clocking = {
    time: null, timeout: 0, running: false,
    text: document.getElementById('clocking-text'),
    update() {
        this.text.textContent = formatTime(Date.now() - this.time);
        this.timeout = setTimeout(() => this.update(), 1000 - (Date.now() % 1000));
    },
    start(time) {
        if (!this.running) {
            if (this.timeout) clearTimeout(this.timeout);
            this.text.textContent = '00:00:00';
            this.time = time;
            this.running = true;
            this.update();
        }
    },
    stop() {
        if (this.running) {
            clearTimeout(this.timeout);
            this.text.textContent = '';
            this.time = Date.now();
            this.running = false;
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

