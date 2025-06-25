import os
import datetime
import tempfile
from sqlalchemy.engine.url import URL

PROPAGATE_EXCEPTIONS = True
DEBUG_HOST = os.getenv("DEBUG_HOST", "127.0.0.1")
DEBUG_PORT = int(os.getenv("DEBUG_PORT", 5000))

APP_NAME = "Zou"
APP_SYSTEM_ERROR_SUBJECT_LINE = "%s system error" % APP_NAME
SECRET_KEY = os.getenv("SECRET_KEY", "mysecretkey")

AUTH_STRATEGY = os.getenv("AUTH_STRATEGY", "auth_local_classic")

KEY_VALUE_STORE = {
    "host": os.getenv("KV_HOST", "localhost"),
    "port": os.getenv("KV_PORT", "6379"),
    "password": os.getenv("KV_PASSWORD", None),
}
AUTH_TOKEN_BLACKLIST_KV_INDEX = 0
MEMOIZE_DB_INDEX = 1
KV_EVENTS_DB_INDEX = 2
KV_JOB_DB_INDEX = 3

JWT_BLACKLIST_ENABLED = True
JWT_BLACKLIST_TOKEN_CHECKS = ["access", "refresh"]
JWT_ACCESS_TOKEN_EXPIRES = datetime.timedelta(days=7)
JWT_REFRESH_TOKEN_EXPIRES = datetime.timedelta(days=15)
JWT_TOKEN_LOCATION = ["cookies", "headers"]
JWT_REFRESH_COOKIE_PATH = "/auth/refresh-token"
JWT_COOKIE_CSRF_PROTECT = False
JWT_SESSION_COOKIE = False
JWT_COOKIE_SAMESITE = "Lax"
JWT_IDENTITY_CLAIM = "sub"

CLIENT_CACHE_MAX_AGE = int(os.getenv("CLIENT_CACHE_MAX_AGE", 604800))

DATABASE = {
    "drivername": os.getenv("DB_DRIVER", "postgresql+psycopg"),
    "host": os.getenv("DB_HOST", "192.168.40.182"),
    "port": os.getenv("DB_PORT", "5432"),
    "username": os.getenv("DB_USERNAME", "postgres"),
    "password": os.getenv("DB_PASSWORD", "euQVpMXeFz8k0A3lD0aj"),
    "database": os.getenv("DB_DATABASE", "zoudb"),
}
SQLALCHEMY_DATABASE_URI = URL.create(**DATABASE).render_as_string(hide_password=False)
SQLALCHEMY_TRACK_MODIFICATIONS = False
SQLALCHEMY_ENGINE_OPTIONS = {
    "pool_size": int(os.getenv("DB_POOL_SIZE", 30)),
    "max_overflow": int(os.getenv("DB_MAX_OVERFLOW", 60)),
}

INDEXER = {
    "host": os.getenv("INDEXER_HOST", "localhost"),
    "port": os.getenv("INDEXER_PORT", "7700"),
    "protocol": os.getenv("INDEXER_PROTOCOL", "http"),
    "key": os.getenv("INDEXER_KEY"),
    "timeout": int(os.getenv("INDEXER_TIMEOUT", 5000)),
}

NB_RECORDS_PER_PAGE = int(os.getenv("NB_RECORDS_PER_PAGE", 100))

PREVIEW_FOLDER = os.getenv(
    "PREVIEW_FOLDER",
    os.getenv("THUMBNAIL_FOLDER", os.path.join(os.getcwd(), "previews")),
)
TMP_DIR = os.getenv("TMP_DIR", os.path.join(tempfile.gettempdir(), "zou"))

EVENT_STREAM_HOST = os.getenv("EVENT_STREAM_HOST", "localhost")
EVENT_STREAM_PORT = os.getenv("EVENT_STREAM_PORT", 5001)
EVENT_HANDLERS_FOLDER = os.getenv(
    "EVENT_HANDLERS_FOLDER", os.path.join(os.getcwd(), "event_handlers")
)

MAIL_SERVER = os.getenv("MAIL_SERVER", "localhost")
MAIL_PORT = os.getenv("MAIL_PORT", 25)
MAIL_USERNAME = os.getenv("MAIL_USERNAME", "")
MAIL_PASSWORD = os.getenv("MAIL_PASSWORD", "")
MAIL_DEFAULT_SENDER = os.getenv(
    "MAIL_DEFAULT_SENDER", "no-reply@your-studio.com"
)
DOMAIN_NAME = os.getenv("DOMAIN_NAME", "localhost:8080")
DOMAIN_PROTOCOL = os.getenv("DOMAIN_PROTOCOL", "https")

PLUGIN_FOLDER = os.getenv(
    "PLUGIN_FOLDER", os.path.join(os.getcwd(), "plugins")
)

FS_BACKEND = os.getenv("FS_BACKEND", "local")
FS_ROOT = PREVIEW_FOLDER
FS_BUCKET_PREFIX = os.getenv("FS_BUCKET_PREFIX", "")
FS_SWIFT_AUTHURL = os.getenv("FS_SWIFT_AUTHURL")
FS_SWIFT_USER = os.getenv("FS_SWIFT_USER")
FS_SWIFT_TENANT_NAME = os.getenv("FS_SWIFT_TENANT_NAME")
FS_SWIFT_KEY = os.getenv("FS_SWIFT_KEY")
FS_SWIFT_REGION_NAME = os.getenv("FS_SWIFT_REGION_NAME")
FS_SWIFT_AUTH_VERSION = os.getenv("FS_SWIFT_AUTH_VERSION", "3")
FS_SWIFT_AES256_KEY = os.getenv("FS_SWIFT_AES256_KEY")
FS_S3_REGION = os.getenv("FS_S3_REGION")
FS_S3_ENDPOINT = os.getenv("FS_S3_ENDPOINT")
FS_S3_ACCESS_KEY = os.getenv("FS_S3_ACCESS_KEY")
FS_S3_SECRET_KEY = os.getenv("FS_S3_SECRET_KEY")
FS_S3_AES256_KEY = os.getenv("FS_S3_AES256_KEY")

JOB_QUEUE_NOMAD_PLAYLIST_JOB = os.getenv(
    "JOB_QUEUE_NOMAD_PLAYLIST_JOB", "zou-playlist"
)
JOB_QUEUE_NOMAD_NORMALIZE_JOB = os.getenv("JOB_QUEUE_NOMAD_NORMALIZE_JOB", "")
JOB_QUEUE_NOMAD_HOST = os.getenv("JOB_QUEUE_NOMAD_HOST", "zou-nomad-01.zou")
JOB_QUEUE_TIMEOUT = os.getenv("JOB_QUEUE_TIMEOUT", 3600)

LDAP_HOST = os.getenv("LDAP_HOST", "127.0.0.1")
LDAP_PORT = os.getenv("LDAP_PORT", "389")
LDAP_BASE_DN = os.getenv("LDAP_BASE_DN", "cn=Users,dc=zou,dc=local")
LDAP_GROUP = os.getenv("LDAP_GROUP", "")
LDAP_DOMAIN = os.getenv("LDAP_DOMAIN", "zou.local")

SAML_IDP_NAME = os.getenv("SAML_IDP_NAME", "")
SAML_METADATA_URL = os.getenv("SAML_METADATA_URL", "")

LOGS_MODE = os.getenv("LOGS_MODE", "default")
LOGS_HOST = os.getenv("LOGS_HOST", "localhost")
LOGS_PORT = os.getenv("LOGS_PORT", 2202)
LOGS_TOKEN = os.getenv("LOGS_TOKEN")

SENTRY_DSN = os.getenv("SENTRY_DSN", "")
SENTRY_SR = float(os.getenv("SENTRY_SR", 1.0))
SENTRY_DEBUG_URL = os.getenv("SENTRY_DEBUG_URL", False)

SENTRY_KITSU_DSN = os.getenv("SENTRY_KITSU_DSN", "")
SENTRY_KITSU_SR = float(os.getenv("SENTRY_KITSU_SR", 0.1))

CRISP_TOKEN = os.getenv("CRISP_TOKEN", "")

DEFAULT_TIMEZONE = os.getenv("DEFAULT_TIMEZONE", "Europe/Paris")
DEFAULT_LOCALE = os.getenv("DEFAULT_LOCALE", "en_US")

USER_LIMIT = int(os.getenv("USER_LIMIT", "100"))
MIN_PASSWORD_LENGTH = int(os.getenv("MIN_PASSWORD_LENGTH", 8))

# Deprecated
TO_REVIEW_TASK_STATUS = "To review"
DEFAULT_FILE_STATUS = "To review"
DEFAULT_FILE_TREE = os.getenv("DEFAULT_FILE_TREE", "default")
