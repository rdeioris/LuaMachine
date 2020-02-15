# Using the HTTP + JSON Functions with RickAndMorty API

LuaMachine includes out of the box UFUNCTION's for interfacing with HTTP services.

In this tututorial we will see how to retrieve JSON data as well as raw images/textures from an HTTP service.

The objective is to build a User Widget listing all of the Rick And Morty characters available at https://rickandmortyapi.com/

## Step 1: adding http_get() and from_json() lua functions

By default a LuaMachine state has no function exposed to the user/scripter. You generally want to carefully choose what to expose to the final scripter/modder, so the default behaviour
is to expose nothing (except the lua standard functions [and this is configurable too]).

We want to expose two functions: one for making http requests (we will call it http_get()) and another for converting json strings to lua tables (we will call it from_json())

Let's add a new LuaState called RickAndMortyLuaState:

## Step 2: adding a LuaUserDataObject representing a RickAndMorty character

## Step 3: creating the widgets

## Step 4: retrieving a characters page from the api

## Step 5: adding character's image

## Step 6: pagination
