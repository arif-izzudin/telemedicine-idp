<script>
  // Import the functions you need from the SDKs you need
  import { initializeApp } from "firebase/app";
  import { getAnalytics, isSupported } from "firebase/analytics";
  // TODO: Add SDKs for Firebase products that you want to use
  // https://firebase.google.com/docs/web/setup#available-libraries

  // Your web app's Firebase configuration
  // For Firebase JS SDK v7.20.0 and later, measurementId is optional
  const firebaseConfig = {
    apiKey: "AIzaSyBjdQ67Naew1jNhzlYZcye7NL5shiCCkfQ",
    authDomain: "telemedicine-idp.firebaseapp.com",
    projectId: "telemedicine-idp",
    storageBucket: "telemedicine-idp.firebasestorage.app",
    messagingSenderId: "950612160792",
    appId: "1:950612160792:web:33c62fabebc4fd21d7ee6d",
    measurementId: "G-441E47BVB6"
  };

  // Initialize Firebase
  const app = initializeApp(firebaseConfig);
  
  // Only initialize analytics on client side
  if (process.client) {
    isSupported().then(supported => {
      if (supported) {
        getAnalytics(app);
      }
    });
  }
</script>

<template>
  <div>
    <NuxtRouteAnnouncer />
    <NuxtWelcome />
  </div>
</template>
