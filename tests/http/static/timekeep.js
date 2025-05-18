function getClockIn() {
    return fetch('/api/timesheet', {
        method: 'GET',
        credentials: 'include'
    })
    .then(response => {
        return response.text().then(text => {
            if (response.ok) {
                return text || null;
            }
            throw new Error(text || `HTTP Error ${response.status}`);
        });
    });
}

async function punch() {
    try {
        const response = await fetch('/api/timesheet', {
            method: 'POST',
            credentials: 'include'
        });

        if (response.status === 200) {
            const data = await response.json();
            user.clock_in = data[1] === '0000-00-00 00:00:00' ? data[0] : null;
        } else {
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        alert(error.message || 'Sesión inválida');
    }
}

