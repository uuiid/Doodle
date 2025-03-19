import traceback
import uuid

from flask import Flask, jsonify, current_app
from flasgger import Swagger
from flask_jwt_extended import JWTManager
from flask_principal import (
    Principal,
    Identity,
    identity_changed,
    RoleNeed,
    UserNeed,
    identity_loaded,
)
from zou.app import config
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from flask_mail import Mail

from jwt import ExpiredSignatureError
from babel.core import UnknownLocaleError
from meilisearch.errors import (
    MeilisearchApiError,
    MeilisearchCommunicationError,
)

app = Flask(__name__)
app.config.from_object(config)
# app["SQLALCHEMY_DATABASE_URI"] = "postgresql://postgres:password@localhost/zoudb"
db = SQLAlchemy(app)
app.extensions["sqlalchemy"].db = db
