$(document).ready(function () {
	var url_pathname = window.location.pathname;
	var tab = url_pathname.split("/")[1];
	changeTabCSS(tab);
});

function changeTabCSS(location){
		$(".tabmenu li").each(function() {
			$(this).removeClass('selected');
			var this_tab = $(this).find("a").attr("href").split("/")[1];
			if(this_tab == location){
				$(this).addClass('selected');
			}
		});
	}