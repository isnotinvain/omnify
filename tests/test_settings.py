from daemomnify.settings import DEFAULT_SETTINGS, DaemomnifySettings


def test_settings_round_trip_serialization():
    """Verify settings can be serialized to JSON and deserialized back."""
    json_str = DEFAULT_SETTINGS.model_dump_json()
    restored = DaemomnifySettings.model_validate_json(json_str)
    assert restored == DEFAULT_SETTINGS
