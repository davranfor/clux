Number.prototype.toMoney = function() {
	var str = this.toFixed(2).toString().split('.');
	
	str[0] = str[0].replace(/\B(?=(\d{3})+(?!\d))/g, '.');
	return str.join(',') + " €";
};
Number.prototype.toNumber = function(decs) {
	var str = this.toFixed(decs).toString().split('.');
	
	str[0] = str[0].replace(/\B(?=(\d{3})+(?!\d))/g, '.');
	return str.join(',');
};

