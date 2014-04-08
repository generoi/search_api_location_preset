

(function ($) {

  var geocoder;

  if (typeof(google) != 'undefined') {
    geocoder = new google.maps.Geocoder();
  }

  Drupal.searchApiLocationPreset = {
    maps: [],
    markers: [],
    circles: [],

    /**
     * Set the latitude and longitude values to the input fields
     * And optionally update the address field
     *
     * @param LatLng
     *   a location (LatLng) object from google maps api
     * @param i
     *   the index from the maps array we are working on
     * @param op
     *   the op that was performed
     */
    codeLatLong: function(LatLng, i, op) {

      // Update the lat and long input fields
      $('#'  + i + '-lat').val(LatLng.lat());
      $('#'  + i + '-long').val(LatLng.lng());

      // Update the address field
      if ((op == 'marker' || op == 'geocoder') && geocoder) {

        geocoder.geocode({ 'latLng' : LatLng }, function(results, status) {

          if (status == google.maps.GeocoderStatus.OK) {
            $("#" + i + "-address").val(results[0].formatted_address);
          }
          else {
            $("#" + i + "-address").val('');
            if (status != google.maps.GeocoderStatus.ZERO_RESULTS) {
              alert(Drupal.t('Geocoder failed due to: ') + status);
            }
          }
        });
      }
    },

    /**
     * Get the location from the address field
     *
     * @param i
     *   the index from the maps array we are working on
     */
    codeAddress: function(i) {
      var address = $("#" + i + "-address").val();

      geocoder.geocode( { 'address': address }, function(results, status) {
        if (status == google.maps.GeocoderStatus.OK) {
          var map = Drupal.searchApiLocationPreset.maps[i];
          if (map) {
            Drupal.searchApiLocationPreset.maps[i].setCenter(results[0].geometry.location);
          }
          Drupal.searchApiLocationPreset.setMapMarker(results[0].geometry.location, i);
          Drupal.searchApiLocationPreset.codeLatLong(results[0].geometry.location, i, 'textinput');
        } else {
          alert(Drupal.t('Geocode was not successful for the following reason: ') + status);
        }
      });
    },

    /**
     * Set/Update a marker on a map
     *
     * @param latLong
     *   a location (latLong) object from google maps api
     * @param i
     *   the index from the maps array we are working on
     */
    setMapMarker: function(LatLng, i) {
      // remove old marker and circle
      if (Drupal.searchApiLocationPreset.markers[i]) {
        Drupal.searchApiLocationPreset.markers[i].setMap(null);
        Drupal.searchApiLocationPreset.circles[i].setMap(null);
      }

      // add marker
      Drupal.searchApiLocationPreset.markers[i] = new google.maps.Marker({
        map: Drupal.searchApiLocationPreset.maps[i],
        draggable: true,
        animation: google.maps.Animation.DROP,
        position: LatLng
      });

      var radius = Drupal.settings.searchApiLocationPreset[i].radius * Drupal.settings.searchApiLocationPreset[i].radius_unit * 1000;
      if ($("#" + i + "-slider").length) {
        radius = $("#" + i + "-slider").slider( "value" ) * Drupal.settings.searchApiLocationPreset[i].radius_unit * 1000;
      }
      // add circle
      //TODO : allow custom colors
      Drupal.searchApiLocationPreset.circles[i] = new google.maps.Circle({
        map: Drupal.searchApiLocationPreset.maps[i],
        clickable:false,
        strokeColor:'#ffcc00',
        fillColor:'#cc3300',
        radius: radius,
        center: LatLng
      });

      // fit the map to te circle
      Drupal.searchApiLocationPreset.maps[i].fitBounds(Drupal.searchApiLocationPreset.circles[i].getBounds());

      return false; // if called from <a>-Tag
    },

    setCurrentCoordinates: function (i) {
      if ('geolocation' in navigator) {
        navigator.geolocation.getCurrentPosition(function(position) {
          // Update the lat and long input fields.
          $('#'  + i + '-lat').val(position.coords.latitude);
          $('#'  + i + '-long').val(position.coords.longitude);
          // Allow other scripts to react when the current position was set.
          $('#'  + i + '-preset').trigger('currentPositionSet', [ position ] );
        }, function(error) {
          Drupal.searchApiLocationPreset.handleGeoError(error, i);
        }, {
          enableHighAccuracy : false,
          // don't wait longer than 5s
          timeout : 5000,
          maximumAge : 0
        });
      }
    },

    handleGeoError: function(error, i) {
      switch(error.code) {
        case error.PERMISSION_DENIED:
          // user denied permission: reset to default search
          //document.location.href = baseUrl;
          alert(Drupal.t('Permission for accessing position denied'));
          break;

        default:
        case error.TIMEOUT:
        case error.POSITION_UNAVAILABLE:
          alert(Drupal.t('Current location is not available'));
      }
      // Allow other scripts to react when the current position was set.
      $('#'  + i + '-preset').trigger('currentPositionError', [ error ] );
    }
  }

  Drupal.behaviors.searchApiLocationPreset = {
    attach: function (context, settings) {
      var lat;
      var long;
      var LatLng;
      var singleClick;

      // Work on each map
      $.each(settings.searchApiLocationPreset, function(i, searchApiLocationPreset) {

        // Create slider if the element is there.
        if ($("#" + i + "-slider", context).length) {
          $("#" + i + "-slider", context).slider({
            value: $("#" + i + "-radius").val(),
            min: parseFloat(searchApiLocationPreset.radius_min),
            max: parseFloat(searchApiLocationPreset.radius_max),
            step: parseFloat(searchApiLocationPreset.radius_step),
            slide: function( event, ui ) {
              $("#"+ i +"-radius").val( ui.value );
              if ($("#"+ i +'-gmap').length) {
                Drupal.searchApiLocationPreset.setMapMarker(Drupal.searchApiLocationPreset.markers[i].getPosition(), i);
              }
            },

            stop: function( event, ui ) {
              $("#"+ i +"-radius").val(ui.value);
            }
          });
          // Initialize value and add change handler.
          $("#" + i + "-radius")
            .val($("#" + i + "-slider").slider("value"))
            .bind('change.searchApiLocationPreset', function( event) {
              $("#" + i + "-slider").slider("value", this.value);
              Drupal.searchApiLocationPreset.setMapMarker(Drupal.searchApiLocationPreset.markers[i].getPosition(), i);
            });
        }

        // Add preset for users current position.
        if (typeof(settings.searchApiLocationPreset[i].add_current_position) != 'undefined' && settings.searchApiLocationPreset[i].add_current_position) {
          if ('geolocation' in navigator) {
            var selected = (settings.searchApiLocationPreset[i].preset == 'current') ? ' selected="selected"': '';
            $("#" + i + "-preset").append('<option value="current" ' + selected + '>' + Drupal.t('Your position') + '</option>');
            $("#" + i + "-preset").bind('change.searchApiLocationPreset', function() {
              if ($(this).val() == 'current') {
                Drupal.searchApiLocationPreset.setCurrentCoordinates(i);
              }
            });
          }
        }

        $("#" + i + "-geocode").click(function(e) {
          Drupal.searchApiLocationPreset.codeAddress(i);
        });
        $("#" + i + "-address").keypress(function(ev){
          // trigger on enter key
          if (ev.which == 13) {
            ev.preventDefault();
            Drupal.searchApiLocationPreset.codeAddress(i);
          }
        });
        $("#" + i + "-places-address").once('process', function() {
          var options = $.extend({
            types: ['geocode'],
          }, searchApiLocationPreset.placesOptions || {});

          var autocomplete = new google.maps.places.Autocomplete(this, options);
          google.maps.event.addListener(autocomplete, 'place_changed', function() {
            var place = autocomplete.getPlace();
            if (typeof place.geometry != 'undefined') {
              Drupal.searchApiLocationPreset.codeLatLong(place.geometry.location, i, 'textinput');
            }
          });
        });

        // Create maps for selecting a location.
        $("#"+ i +'-gmap').once('process', function(){
          lat = parseFloat(searchApiLocationPreset.lat);
          long = parseFloat(searchApiLocationPreset.long);

          LatLng = new google.maps.LatLng(lat, long);

          // Create map
          Drupal.searchApiLocationPreset.maps[i] = new google.maps.Map(document.getElementById(i + "-gmap"), {
            zoom: 2,
            center: LatLng,
            mapTypeId: google.maps.MapTypeId.ROADMAP,
            scrollwheel: false
          });

          if (lat && long) {
            // Set initial marker
            Drupal.searchApiLocationPreset.setMapMarker(LatLng, i);
            Drupal.searchApiLocationPreset.codeLatLong(LatLng, i, 'geocoder');
          }

          // Listener to click
          google.maps.event.addListener(Drupal.searchApiLocationPreset.maps[i], 'click', function(me){
            // Set a timeOut so that it doesn't execute if dbclick is detected
            singleClick = setTimeout(function(){
              Drupal.searchApiLocationPreset.codeLatLong(me.LatLng, i, 'marker');
              Drupal.searchApiLocationPreset.setMapMarker(me.LatLng, i);
            }, 500);
          });

          // Detect double click to avoid setting marker
          google.maps.event.addListener(Drupal.searchApiLocationPreset.maps[i], 'dblclick', function(me){
            clearTimeout(singleClick);
          });

          // Listener to dragged.
          google.maps.event.addListener(Drupal.searchApiLocationPreset.markers[i], 'dragend', function(me){
            Drupal.searchApiLocationPreset.codeLatLong(me.LatLng, i, 'marker');
            Drupal.searchApiLocationPreset.setMapMarker(me.LatLng, i);
          });

        })
      });
    }
  }
}
)(jQuery);

