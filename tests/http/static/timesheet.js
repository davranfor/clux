function getClockInTime() {
    return fetch('/api/timesheet', {
        method: 'GET',
        credentials: 'include'
    })
    .then(response => {
        return response.text().then(text => {
            if (response.ok) {
                return text || 0;
            }
            throw new Error(text || `HTTP Error ${response.status}`);
        });
    });
}

async function logHours() {
    try {
        const response = await fetch('/api/timesheet', {
            method: 'POST',
            credentials: 'include'
        });

        if (response.status === 200) {
            const data = await response.json();
            user.clockInTime = data[1] === 0 ? data[0] : 0;
        } else if (response.status === 204) {
            throw new Error('Deben pasar al menos 5 segundos entre entrada y salida');
        } else {
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        throw new Error(error.message || `HTTP Error ${response.status}`);
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
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        throw new Error(error.message || `HTTP Error ${response.status}`);
    }
}

