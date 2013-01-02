var colorMain = 'rgb(160, 160, 160)';
var colorBlank = 'rgb(255, 255, 255)';
var colorAccent = 'rgb(6, 174, 189)';
var animSpeed = 160;

// Fixed main menu
$(document).scroll(function() {
    var useFixedSidebar = $(document).scrollTop() >= 60;
    
    $('.mainMenu').toggleClass('fixedMainMenu', useFixedSidebar);

    if(useFixedSidebar) $('.main').css({"margin-top": "60px"});  
    else $('.main').css.get({"margin-top": "0px"});	  
});

// Menu buttons
$('.menuButton').hover(
	function () {
   		$(this).stop().animate({backgroundColor: colorBlank}, animSpeed);
    	$(this).find('a').stop().animate({color: colorMain}, animSpeed);
  	},
  	function () {
	  	$(this).stop().animate({backgroundColor: colorMain}, animSpeed);
	  	$(this).find('a').stop().animate({color: colorBlank}, animSpeed);
  	}
);

// Social icons
$('.socialIcon').hover(
	function () {
   		$(this).stop().animate({backgroundColor: colorBlank}, animSpeed);
   		$(this).find('.socialIconGray').stop().css({opacity: 0.0}).animate({opacity: 1.0});
  	},
  	function () {
	  	$(this).stop().animate({backgroundColor: colorMain}, animSpeed);
	  	$(this).find('.socialIconGray').stop().css({opacity: 1.0}).animate({opacity: 0.0});
  	}
);

