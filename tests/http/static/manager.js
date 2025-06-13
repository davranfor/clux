const profile = {
    id: profileForm.querySelector('input[name="id"]'),
    workplace: profileForm.querySelector('select[name="workplace"]'),
    role: profileForm.querySelector('select[name="role"]'),
    name: profileForm.querySelector('input[name="name"]'),
    tin: profileForm.querySelector('input[name="tin"]'),
    address: profileForm.querySelector('textarea[name="address"]'),
    phone: profileForm.querySelector('input[name="phone"]'),
    email: profileForm.querySelector('input[name="email"]')
}

async function profileEdit(id) {
    const response = await fetch(`/api/users/${id}/edit`, {
        method: 'GET',
        credentials: 'include'
    });

    if (response.status === 200) {
        const data = await response.json();
        profileEditUI(data);
    } else if (response.status === 204) {
        showMessage('El registro ya no existe');
    } else {
        const text = await response.text();
        throw new Error(text || `HTTP Error ${response.status}`);
    }
}

function profileEditUI(data) {
    profile.id.value = data.id;
    profile.workplace.replaceChildren();
    data.workplaces.forEach(workplace => {
        const option = document.createElement('option');

        option.value = workplace[0];
        option.textContent = workplace[1];
        profile.workplace.appendChild(option);
    });
    profile.workplace.value = data.workplace_id;
    profile.role.value = data.role;
    profile.name.value = data.name;
    profile.tin.value = data.tin;
    profile.address.value = data.address;
    profile.phone.value = data.phone;
    profile.email.value = data.email;
}

document.getElementById("profile-cancel").addEventListener("click", () => {
    profileEdit(user.id).catch(error => { if (error.message) showMessage(error.message); });
});

